#pragma once

#include "ComponentWindow.h"

class ComponentTransform;

enum class Axis
{
	X,
	Y,
	Z
};

class WindowComponentTransform : public ComponentWindow
{
public:
	WindowComponentTransform(ComponentTransform* component);
	~WindowComponentTransform() override;

protected:
	void DrawWindowContents() override;

private:
	void DrawTransformTable();
	void UpdateComponentTransform();

	float3 currentTranslation;
	float3 currentRotation;
	float3 currentScale;
	float currentDragSpeed;
	bool translationModified;
	bool rotationModified;
	bool scaleModified;
	Axis modifiedScaleAxis;

	float3 bbTranslation;
	float3 bbScale;

	bool bbdraw;
	bool uniformScale;
};
