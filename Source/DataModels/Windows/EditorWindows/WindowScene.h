#pragma once

#include "EditorWindow.h"

#include "GL/glew.h"

#include "ImGui/ImGuizmo.h"

#include "Math/float4x4.h"

#define VIEW_MANIPULATE_SIZE 128
#define VIEW_MANIPULATE_TOP_PADDING 35

class WindowScene : public EditorWindow
{
public:
	WindowScene();
	~WindowScene() override;

	ImVec2 GetStartPos() const;
	ImVec2 GetEndPos() const;
	ImVec2 GetAvailableRegion() const;

	bool IsMouseInsideManipulator(float x, float y) const;

protected:
	void DrawWindowContents() override;

private:
	void ManageResize();
	void DrawGuizmo();
	void DrawSceneMenu();

	GLuint texture;
	float currentWidth;
	float currentHeight;

	ImGuizmo::OPERATION gizmoCurrentOperation;
	ImGuizmo::MODE gizmoCurrentMode;

	bool useSnap;
	bool manipulatedLastFrame;

	float4x4 manipulatedViewMatrix;
	float3 snap;

	ImVec2 availableRegion;
	ImVec2 viewportBounds[2]; // [0] minViewport, [1] maxViewport
};

inline ImVec2 WindowScene::GetStartPos() const
{
	return viewportBounds[0];
}

inline ImVec2 WindowScene::GetEndPos() const
{
	return viewportBounds[1];
}

inline ImVec2 WindowScene::GetAvailableRegion() const
{
	return availableRegion;
}

inline bool WindowScene::IsMouseInsideManipulator(float x, float y) const
{
	if (!IsFocused())
	{
		return false;
	}
	return x <= viewportBounds[1].x && x >= viewportBounds[1].x - VIEW_MANIPULATE_SIZE && y >= viewportBounds[0].y &&
		   y <= viewportBounds[0].y + VIEW_MANIPULATE_SIZE;
}