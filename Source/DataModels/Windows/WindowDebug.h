#pragma once

#include "Window.h"

class WindowDebug : public Window
{
public:
	WindowDebug();
	~WindowDebug() override;

	void Draw(bool& enabled = defaultEnabled) override;

	const bool GetDrawBoundingBoxes() const;
	const bool GetDrawAreaLight() const;
	const bool GetDrawSpotLight() const;
	const bool GetDrawPointLight() const;
	const bool GetDrawDirLight() const;
	const bool GetDrawPhysics() const;

private:
	static bool defaultEnabled;
	bool wireframeMode = false;
	bool drawBoundingBoxes = false;
	bool drawAreaLight = false;
	bool drawSpotLight = false;
	bool drawDirLight = false;
	bool drawPointLight = false;
	bool drawPhysics = false;
};

inline const bool WindowDebug::GetDrawBoundingBoxes() const
{
	return drawBoundingBoxes;
}

inline const bool WindowDebug::GetDrawAreaLight() const
{
	return drawAreaLight;
}

inline const bool WindowDebug::GetDrawSpotLight() const
{
	return drawSpotLight;
}

inline const bool WindowDebug::GetDrawPointLight() const
{
	return drawPointLight;
}

inline const bool WindowDebug::GetDrawDirLight() const
{
	return drawDirLight;
}

inline const bool WindowDebug::GetDrawPhysics() const
{
	return drawPhysics;
}
