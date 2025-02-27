#pragma once
#include "ComponentWindow.h"

class ComponentLine;
class ResourceTexture;
class WindowLineTexture;

struct ImGradientMark;

class WindowComponentLine : public ComponentWindow
{
public:
	WindowComponentLine(ComponentLine* component);
	~WindowComponentLine() override;

	void SetTexture(const std::shared_ptr<ResourceTexture>& texture);
	void InitValues();

protected:
	void DrawWindowContents() override;

private:

	float2 tiling;
	float2 offset;

	std::unique_ptr<WindowLineTexture> inputTexture;
	std::shared_ptr<ResourceTexture> lineTexture;

	ImGradientMark* draggingMark;
	ImGradientMark* selectedMark;

};


inline void WindowComponentLine::SetTexture(const std::shared_ptr<ResourceTexture>& texture)
{
	lineTexture = texture;
}
