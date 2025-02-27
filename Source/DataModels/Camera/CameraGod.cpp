#include "StdAfx.h"

#include "CameraGod.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleWindow.h"

CameraGod::CameraGod() : Camera(CameraType::C_GOD)
{
}

CameraGod::CameraGod(const std::unique_ptr<Camera>& camera) : Camera(camera, CameraType::C_GOD)
{
}

CameraGod::~CameraGod()
{
}

bool CameraGod::Update()
{
	ModuleInput* input = App->GetModule<ModuleInput>();
	if (input->GetInFocus())
	{
		projectionMatrix = frustum->ProjectionMatrix();
		viewMatrix = frustum->ViewMatrix();

		// Shift speed
		if (input->GetKey(SDL_SCANCODE_LSHIFT) != KeyState::IDLE)
		{
			Run();
		}
		else
		{
			Walk();
		}
		// Move and rotate with right buttons and ASDWQE
		if (!App->IsDebuggingGame())
		{
			KeepMouseCentered();
			Move();
			if (!backFromDebugging)
			{
				FreeLook();
			}
			backFromDebugging = false;
		}
		else
		{
			backFromDebugging = true;
		}

		KeyboardRotate();

		if (frustumMode == EFrustumMode::offsetFrustum)
		{
			RecalculateOffsetPlanes();
		}
	}

	return true;
}

void CameraGod::Move()
{
	ModuleInput* input = App->GetModule<ModuleInput>();

	float deltaTime = App->GetDeltaTime();

	// Forward
	if (input->GetKey(SDL_SCANCODE_W) != KeyState::IDLE)
	{
		position += frustum->Front().Normalized() * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}

	// Backward
	if (input->GetKey(SDL_SCANCODE_S) != KeyState::IDLE)
	{
		position += -(frustum->Front().Normalized()) * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}

	// Left
	if (input->GetKey(SDL_SCANCODE_A) != KeyState::IDLE)
	{
		position += -(frustum->WorldRight()) * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}

	// Right
	if (input->GetKey(SDL_SCANCODE_D) != KeyState::IDLE)
	{
		position += frustum->WorldRight() * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}

	// Up
	if (input->GetKey(SDL_SCANCODE_E) != KeyState::IDLE)
	{
		position += frustum->Up() * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}

	// Down
	if (input->GetKey(SDL_SCANCODE_Q) != KeyState::IDLE)
	{
		position += -(frustum->Up()) * moveSpeed * acceleration * deltaTime;
		SetPosition(position);
	}
}

void CameraGod::KeepMouseCentered()
{
	ModuleInput* input = App->GetModule<ModuleInput>();

	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	input->SetMouseMotionX(float(mouseX - lastMouseX));
	input->SetMouseMotionY(float(mouseY - lastMouseY));

	int width, height;
	SDL_Window* sdlWindow = App->GetModule<ModuleWindow>()->GetWindow();
	SDL_GetWindowSize(sdlWindow, &width, &height);
	SDL_WarpMouseInWindow(sdlWindow, width / 2, height / 2);

	lastMouseX = width / 2;
	lastMouseY = height / 2;
}