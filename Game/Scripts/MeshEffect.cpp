#include "StdAfx.h"

#include "MeshEffect.h"

#include "Application.h"

#include "Components/ComponentMeshRenderer.h"
#include "Components/ComponentScript.h"

REGISTERCLASS(MeshEffect);

MeshEffect::MeshEffect() : Script(), maxTime(0.f), timeBetween(0.f), effectTime(0.f), activateEffect(false)
{
}

void MeshEffect::Start()
{
}

void MeshEffect::Update(float deltaTime)
{
}

void MeshEffect::FillMeshes(GameObject* parent)
{
	ComponentMeshRenderer* mesh = parent->GetComponentInternal<ComponentMeshRenderer>();
	if (mesh)
	{
		meshes.push_back(mesh);
	}

	for (GameObject* child : parent->GetChildren())
	{
		FillMeshes(child);
	}
}

void MeshEffect::StartEffect(float maxTime, float timeBetween)
{
	this->timeBetween = timeBetween;
	this->maxTime = maxTime;
	activateEffect = true;
	effectTime = 0.f;
}

void MeshEffect::ClearEffect()
{
	for (auto mesh : meshes)
	{
		mesh->SetDiscard(false);
		mesh->SetEffectColor(float4(0.f, 0.f, 0.f, 0.f));
	}
}

void MeshEffect::DamageEffect()
{
	if (!activateEffect)
	{
		return;
	}

	if (meshes.empty())
	{
		FillMeshes(owner);
	}

	if (effectTime == 0.f)
	{
		EffectColor(colors[0]);
	}
	effectTime += App->GetDeltaTime();
	if (effectTime > timeBetween)
	{
		EffectDiscard();
	}
	if (effectTime > maxTime)
	{
		activateEffect = false;
		ClearEffect();
	}
}

void MeshEffect::FadeEffect()
{
	if (!activateEffect)
	{
		return;
	}

	effectTime += App->GetDeltaTime();
	float step = 0.1f / maxTime;
	colors[0].w -= step;
	EffectColor(colors[0]);
	if (effectTime >= maxTime)
	{
		activateEffect = false;
		ClearEffect();
	}
}

void MeshEffect::DisappearBodyEffect()
{
	if (!activateEffect)
	{
		return;
	}

	if (meshes.empty())
	{
		FillMeshes(owner);
	}
	
	colors[0].w -= 0.02f;
	EffectColor(colors[0]);
	
	effectTime += App->GetDeltaTime();
	
	if (effectTime > maxTime)
	{
		activateEffect = false;
		ClearEffect();
	}
}

void MeshEffect::MakeNonReflective()
{
	if (meshes.empty())
	{
		FillMeshes(owner);
	}

	for (auto mesh : meshes)
	{
		mesh->SetReflective(true);
	}
}

void MeshEffect::MakeReflective()
{
	if (meshes.empty())
	{
		FillMeshes(owner);
	}

	for (auto mesh : meshes)
	{
		mesh->SetReflective(false);
	}
}

void MeshEffect::EnableDisableMeshes(bool enable)
{
	if (meshes.empty())
	{
		FillMeshes(owner);
	}

	for (auto mesh : meshes)
	{
		if (!enable)
		{
			mesh->SetReflective(true);
			mesh->Disable();
		}
		else
		{
			mesh->Enable();
			mesh->SetReflective(false);
		}
	}
}

void MeshEffect::EffectDiscard()
{
	for (auto mesh : meshes)
	{
		mesh->SetEffectColor(float4(0.f, 0.f, 0.f, 0.f));
		mesh->SetDiscard(true);
	}
}

void MeshEffect::EffectColor(float4 color)
{
	for (auto mesh : meshes)
	{
		mesh->SetDiscard(false);
		mesh->SetEffectColor(color);
	}
}