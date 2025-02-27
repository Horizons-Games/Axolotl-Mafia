#include "StdAfx.h"

#include "WindowLoadScene.h"

#include "Application.h"
#include "Auxiliar/Utils/ConvertU8String.h"
#include "ModuleScene.h"

#include "Defines/ExtensionDefines.h"

WindowLoadScene::WindowLoadScene() : WindowFileBrowser(), loadingScene(false)
{
	dialogName = " Load Scene";
	title = ConvertU8String(ICON_IGFD_FOLDER) + " Load Scene";
	filters = SCENE_EXTENSION;
	startPath = "Assets/Scenes";
}

WindowLoadScene::~WindowLoadScene()
{
}

void WindowLoadScene::DoThisIfOk()
{
	this->isLoading = false;
	loadingScene = true;
	std::string filePath = std::string(fileDialogImporter.GetFilePathName());

	std::chrono::time_point currentTime = std::chrono::system_clock::now();
	App->GetModule<ModuleScene>()->LoadSceneAsync(
		filePath,
		[currentTime, this]()
		{
			loadingScene = false;
			long long duration =
				std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - currentTime)
					.count();
			LOG_DEBUG("Scene Loaded! Took {} seconds", static_cast<int>(duration));
		});
}