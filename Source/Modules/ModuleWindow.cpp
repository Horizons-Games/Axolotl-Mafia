#include "StdAfx.h"

#include "ModuleWindow.h"

#include "Defines/WindowDefines.h"
#include "Defines/ApplicationDefines.h"

ModuleWindow::ModuleWindow() : fullscreen(false), brightness(0.0f)
{
}

ModuleWindow::~ModuleWindow()
{
}

bool ModuleWindow::Init()
{
	LOG_VERBOSE("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		LOG_ERROR("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		SDL_DisplayMode DM;
		SDL_GetCurrentDisplayMode(0, &DM);
		int width = DM.w;
		int height = DM.h - TOP_WINDOWED_PADDING;

		brightness = BRIGHTNESS;

		Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED;

		if (FULLSCREEN)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		SDL_Window* windowRawPointer =
			SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);

		window = std::unique_ptr<SDL_Window, SDLWindowDestroyer>(windowRawPointer);
		SetVsync(false);

		if (window == nullptr)
		{
			LOG_ERROR("Window could not be created! SDL_Error: {}\n", SDL_GetError());
			ret = false;
		}
		else
		{
			SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
			// Get window surface
			screenSurface = SDL_GetWindowSurface(window.get());
		}
		SetResizable(true);
	}

	return ret;
}

bool ModuleWindow::CleanUp()
{
	LOG_VERBOSE("Destroying SDL window and quitting all SDL systems");

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

std::pair<int, int> ModuleWindow::GetWindowSize() const
{
	int width, height;

	SDL_GetWindowSize(GetWindow(), &width, &height);

	return std::make_pair(width, height);
}

void ModuleWindow::SetWindowSize(int width, int height)
{
	SDL_SetWindowSize(GetWindow(), width, height);
}

void ModuleWindow::SetWindowToDefault()
{
	LOG_VERBOSE("---- Changing window mode ----");

	SDL_SetWindowFullscreen(GetWindow(), 0);
	SDL_SetWindowResizable(GetWindow(), SDL_FALSE);
	SDL_SetWindowBordered(GetWindow(), SDL_TRUE);
}

void ModuleWindow::SetFullscreen(bool fullscreen)
{
	SetWindowToDefault();
	if (fullscreen)
	{
		SDL_SetWindowFullscreen(GetWindow(), SDL_WINDOW_FULLSCREEN);
		this->fullscreen = true;
	}
}

void ModuleWindow::SetResizable(bool resizable)
{
	SetWindowToDefault();
	SDL_SetWindowResizable(GetWindow(), BoolToSDL_Bool(resizable));
}

void ModuleWindow::SetBorderless(bool borderless)
{
	SetWindowToDefault();
	// this call sets borders, so it's the opposite of what we want
	// thus the negation
	SDL_SetWindowBordered(GetWindow(), BoolToSDL_Bool(!borderless));
}

void ModuleWindow::SetDesktopFullscreen(bool fullDesktop)
{
	SetWindowToDefault();
	if (fullDesktop)
	{
		SDL_SetWindowFullscreen(GetWindow(), SDL_WINDOW_FULLSCREEN_DESKTOP);
		fullscreen = false;
	}
}

void ModuleWindow::SetBrightness(float brightness)
{
	this->brightness = brightness;

	if (SDL_SetWindowBrightness(GetWindow(), brightness))
	{
		LOG_ERROR("Error setting window brightness: {}", &SDL_GetError()[0]);
	}
}

void ModuleWindow::SetVsync(bool vsyncactive)
{
	vsync = vsyncactive;
	SDL_GL_SetSwapInterval(vsyncactive);
}
