#include "StdAfx.h"

#include "Application.h"
#include "FileSystem/ModuleFileSystem.h"

#include <mutex>

namespace
{
const char* documentsPath = "Documents";
const char* logFilePath = "Documents/Axolotl.log";

std::recursive_mutex writeLock;
} // namespace

void AxoLog::Write(const char file[], int line, LogSeverity severity, std::string&& formattedLine)
{
	// Lock the mutex so the function is thread-safe
	// this scoped_lock will be destroyed upon exiting the method, thus freeing the mutex
	// according to the documentation, this is deadlock-safe,
	// meaning that there won't be any deadlocks if the mutex is attempted to be locked twice
	std::scoped_lock lock(writeLock);

	LogLine logLine{ severity, file, static_cast<uint16_t>(line), std::move(formattedLine) };
	logLines.push_back(logLine);

	std::string detailedString = logLine.ToDetailedString();

	OutputDebugStringA(detailedString.c_str());

	bool recursiveLogCall = file == __FILE__;
	if (writingToFile && !recursiveLogCall)
	{
		if (App == nullptr || App->GetModule<ModuleFileSystem>() == nullptr)
		{
			LOG_ERROR("Error writing to log file, FileSystem already terminated");
			return;
		}
		if (App->GetModule<ModuleFileSystem>()->Save(
				logFilePath, detailedString.c_str(), detailedString.size(), true) == 1)
		{
			LOG_ERROR("Error writing to log file");
		}
	}
}

void AxoLog::StartWritingToFile()
{
	assert(App);
	const ModuleFileSystem* fileSystem = App->GetModule<ModuleFileSystem>();
	assert(fileSystem);
	if (!fileSystem->Exists(documentsPath))
	{
		fileSystem->CreateDirectory(documentsPath);
	}
	// if folder does not exist, we know for sure the file won't either
	else if (fileSystem->Exists(logFilePath))
	{
		if (!fileSystem->Delete(logFilePath))
		{
			LOG_ERROR("FileSystem error; no logging will be saved this execution");
			return;
		}
	}
	for (std::string line : logLines | std::views::transform(
										   [](const LogLine& logLine)
										   {
											   return logLine.ToDetailedString();
										   }))
	{
		if (App->GetModule<ModuleFileSystem>()->Save(logFilePath, line.c_str(), line.size(), true) == 1)
		{
			LOG_ERROR("FileSystem error; no logging will be saved this execution");
			return;
		}
	}
	writingToFile = true;
}

void AxoLog::StopWritingToFile()
{
	LOG_INFO("Closing writer...");
	writingToFile = false;
}

//////////////////////////////////////////////////////////////////////////
// LogLine definitions
//////////////////////////////////////////////////////////////////////////

std::string AxoLog::LogLine::ToDetailedString(bool addBreak) const
{
	return ToString(true, addBreak);
}

std::string AxoLog::LogLine::ToSimpleString(bool addBreak) const
{
	return ToString(false, addBreak);
}

std::string AxoLog::LogLine::ToString(bool detailed, bool addBreak) const
{
	std::string result;
	switch (severity)
	{
		case LogSeverity::INFO_LOG:
			result = "[INFO]";
			break;
		case LogSeverity::VERBOSE_LOG:
			result = "[VERBOSE]";
			break;
		case LogSeverity::DEBUG_LOG:
			result = "[DEBUG]";
			break;
		case LogSeverity::WARNING_LOG:
			result = "[WARNING]";
			break;
		case LogSeverity::ERROR_LOG:
			result = "[ERROR]";
			break;
	}
	if (detailed)
	{
		result += file + "(" + std::to_string(line) + ") : ";
	}
	result += message;
	if (addBreak)
	{
		result += '\n';
	}
	return result;
}
