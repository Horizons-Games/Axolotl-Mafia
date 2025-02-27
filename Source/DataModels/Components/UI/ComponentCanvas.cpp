#include "StdAfx.h"

#include "ComponentCanvas.h"

#include "Application.h"
#include "FileSystem/Json.h"

#include "GameObject/GameObject.h"

#include "ModuleEditor.h"
#include "ModuleWindow.h"

#include "Windows/EditorWindows/WindowScene.h"

ComponentCanvas::ComponentCanvas(bool active, GameObject* owner) :
	Component(ComponentType::CANVAS, active, owner, true),
	screenReferenceSize(float2(1920, 1080)),
	size(float2(1920, 1080)),
	screenFactor(0.0f)
{
	RecalculateSizeAndScreenFactor();
}

ComponentCanvas::~ComponentCanvas()
{
}

void ComponentCanvas::InternalSave(Json& meta)
{
}

void ComponentCanvas::InternalLoad(const Json& meta)
{
	RecalculateSizeAndScreenFactor();
}

void ComponentCanvas::RecalculateSizeAndScreenFactor()
{
	std::pair<float, float> region = App->GetModule<ModuleEditor>()->GetAvailableRegion();
	size = float2(floor(region.first), floor(region.second));
	float2 factor = size.Div(screenReferenceSize);
	screenFactor = factor.x < factor.y ? factor.x : factor.y;
}