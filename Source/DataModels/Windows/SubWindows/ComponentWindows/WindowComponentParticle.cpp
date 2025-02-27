#include "StdAfx.h"

#include "WindowComponentParticle.h"

#include "Windows/EditorWindows/ImporterWindows/WindowParticleSystemInput.h"
#include "Resources/ResourceParticleSystem.h"
#include "Components/ComponentParticleSystem.h"

#include "ParticleSystem/EmitterInstance.h"
#include "ParticleSystem/ParticleEmitter.h"
#include "ParticleSystem/ParticleModule.h"

#include "Application.h"
#include "FileSystem/ModuleResources.h"
#include "ModuleScene.h"

#include <vector>

WindowComponentParticle::WindowComponentParticle(ComponentParticleSystem* component) :
	ComponentWindow("PARTICLE SYSTEM", component), 
	inputParticleSystem(std::make_unique<WindowParticleSystemInput>(component))
{
}

WindowComponentParticle::~WindowComponentParticle()
{
}

void WindowComponentParticle::DrawWindowContents()
{
	DrawEnableAndDeleteComponent();

	if (!component)
	{
		return;
	}

	ImGui::Text("");

	ComponentParticleSystem* component = static_cast<ComponentParticleSystem*>(this->component);
	std::shared_ptr<ResourceParticleSystem> resource = component->GetResource();
	if(resource)
	{
		ImGui::Text(resource->GetFileName().c_str());
		ImGui::SameLine();
		if (ImGui::Button("x"))
		{
			component->SetResource(nullptr);
			return;
		}

		if (ImGui::Button("Save"))
		{
			component->SaveConfig();
			resource->SetChanged(true);
			App->GetModule<ModuleResources>()->ReimportResource(resource->GetUID());
			App->GetModule<ModuleScene>()->ParticlesSystemUpdate(true);
			return;
		}
		ImGui::SameLine();
		bool playAtStart = component->GetPlayAtStart();
		if (ImGui::Checkbox("Play at start", &playAtStart))
		{
			component->SetPlayAtStart(playAtStart);
		}

		int id = 0;

		if (component->GetEmitters().size() > 0)
		{
			ImGui::Separator();

			if (ImGui::ArrowButton("##Play", ImGuiDir_Right))
			{
				if (!component->IsPlaying())
				{
					component->Play();
				}
				else
				{
					component->Stop();
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("||"))
			{
				if (component->IsPlaying())
				{
					component->Pause();
				}
			}

			ImGui::Separator();
		}

		std::vector<EmitterInstance*> emitters = component->GetEmitters();

		int emitterToRemove = -1;

		for (int id = 0; id < emitters.size(); ++id)
		{
			ImGui::PushID(id);

			if (ImGui::Button("^"))
			{
				if (id > 0)
				{
					std::swap(emitters[id], emitters[id - 1]);
					component->SetEmitters(emitters);
					component->GetResource()->SwapEmitter(id, id - 1);
					ImGui::PopID();
					return;
				} 
			}
			ImGui::SameLine();
			if (ImGui ::Button("v"))
			{
				if (id < emitters.size() - 1)
				{
					std::swap(emitters[id], emitters[id + 1]);
					component->SetEmitters(emitters);
					component->GetResource()->SwapEmitter(id, id + 1);
					ImGui::PopID();
					return;
				}
			}

			DrawEmitter(emitters[id]);

			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			if (ImGui::Button("Delete Emitter", ImVec2(115.0f, 20.0f)))
			{
				emitterToRemove = id;
			}
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			ImGui::Separator();

			ImGui::PopID();
		}

		if (emitterToRemove != -1)
		{
			resource->RemoveEmitter(emitterToRemove);
			App->GetModule<ModuleScene>()->ParticlesSystemUpdate();
			return;
		}

		if (ImGui::Button("Add an Emitter"))
		{
			std::unique_ptr<ParticleEmitter> emitter = std::make_unique<ParticleEmitter>();
			//This is naming is not correct but we will do it like this for the moment
			std::string name = "DefaultEmitter_" + std::to_string(component->GetEmitters().size());

			emitter->SetName(&name[0]);
			//You need to update the resource and THEN all the components detect that change and update their instances
			resource->AddEmitter(std::move(emitter));
			App->GetModule<ModuleScene>()->ParticlesSystemUpdate();
		}
	}
	else
	{
		inputParticleSystem->DrawWindowContents();
	}
}

void WindowComponentParticle::DrawEmitter(EmitterInstance* instance)
{
	if (instance)
	{
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(0.0f, 2.5f));
		std::string name = instance->GetName().c_str();
		if (ImGui::InputText(("##" + name).c_str(), name.data(), 64, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			instance->SetName(name.c_str());
		}

		bool open = instance->IsVisibleConfig();

		ParticleEmitter::ShapeType shape = instance->GetShape();

		const char* items[] = { "CIRCLE", "CONE", "BOX" };

		static const char* currentItem;
		switch (shape)
		{
		case ParticleEmitter::ShapeType::CIRCLE:
			currentItem = items[0];
			break;
		case ParticleEmitter::ShapeType::CONE:
			currentItem = items[1];
			break;
		case ParticleEmitter::ShapeType::BOX:
			currentItem = items[2];
			break;
		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f));

		if (ImGui::BeginTable("ParametersTable", 2))
		{
			ImGui::TableNextColumn();
			ImGui::Text("Shape");
			/*ImGui::SameLine(0.0f, 10.0f);*/
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(120.0f);

			if (ImGui::BeginCombo("##shapeCombo", currentItem))
			{
				for (int n = 0; n < IM_ARRAYSIZE(items); ++n)
				{
					// You can store your selection however you want, outside or inside your objects
					bool isSelected = (currentItem == items[n]);
					if (ImGui::Selectable(items[n], isSelected))
					{
						currentItem = items[n];
						if (isSelected)
						{
							// You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
							ImGui::SetItemDefaultFocus();
						}
					}
				}

				if (currentItem == items[0])
				{
					shape = ParticleEmitter::ShapeType::CIRCLE;
				}
				else if (currentItem == items[1])
				{
					shape = ParticleEmitter::ShapeType::CONE;
				}
				else if (currentItem == items[2])
				{
					shape = ParticleEmitter::ShapeType::BOX;
				}

				instance->SetShape(shape);

				ImGui::EndCombo();
			}

			int maxParticles = instance->GetMaxParticles();
			float angle = instance->GetAngle();
			float radius = instance->GetRadius();
			float duration = instance->GetDuration();
			float2 lifespanRange = instance->GetLifespanRange();
			float2 speedRange = instance->GetSpeedRange();
			float2 sizeRange = instance->GetSizeRange();
			float2 rotRange = instance->GetRotationRange();
			float2 gravRange = instance->GetGravityRange();
			float4 color = instance->GetColor();

			bool isLooping = instance->IsLooping();
			bool randomLife = instance->IsRandomLife();
			bool randomSpeed = instance->IsRandomSpeed();
			bool randomSize = instance->IsRandomSize();
			bool randomRot = instance->IsRandomRot();
			bool randomGrav = instance->IsRandomGravity();
			bool lifespanModif = false;
			bool speedModif = false;
			bool sizeModif = false;
			bool rotModif = false;
			bool gravModif = false;

			ImGui::TableNextColumn();
			ImGui::Text("Shape config");
			ImGui::TableNextColumn();
			ImGui::Text("Radius:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::DragFloat("##radius", &radius, 0.1f, MIN_RADIUS, MAX_RADIUS, "%.3f"))
			{
				if (radius > MAX_RADIUS)
				{
					radius = MAX_RADIUS;
				}
				else if (radius < MIN_RADIUS)
				{
					radius = MIN_RADIUS;
				}
				instance->SetRadius(radius);
			}
			if (shape == ParticleEmitter::ShapeType::CONE)
			{
				ImGui::SameLine();
				ImGui::Text("Angle:"); ImGui::SameLine();
				ImGui::SetNextItemWidth(60.0f);
				if (ImGui::DragFloat("##angle", &angle, 0.01f, MIN_ANGLE, MAX_ANGLE, "%.2f"))
				{
					instance->SetAngle(angle);
				}
			}

			ImGui::Dummy(ImVec2(0.0f, 2.5f));

			ImGui::TableNextColumn();
			ImGui::Text("Max particles");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (ImGui::InputInt("##maxParticles", &maxParticles, 1, 5))
			{
				if (maxParticles > MAX_PARTICLES)
				{
					maxParticles = MAX_PARTICLES;
				}
				else if (maxParticles < 0)
				{
					maxParticles = 0;
				}
				instance->SetMaxParticles(maxParticles);
				instance->Init();
			}

			ImGui::TableNextColumn();
			ImGui::Text("Duration");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (ImGui::InputFloat("##duration", &duration, 1, 5, "%.2f"))
			{
				if (duration > MAX_DURATION)
				{
					duration = MAX_DURATION;
				}
				else if (duration < 0.0f)
				{
					duration = 0.0f;
				}
				instance->SetDuration(duration);
			}

			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Loop");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##isLooping", &isLooping))
			{
				instance->SetLooping(isLooping);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Elapsed time");
			ImGui::TableNextColumn();
			ImGui::Text("%f seconds", instance->GetElapsedTime());

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::TableNextColumn();
			ImGui::Text("Lifespan");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (randomLife)
			{
				if (ImGui::DragFloat2("##sliderlife", &lifespanRange[0], 0.1f, 0.00f, MAX_DURATION, "%.2f"))
				{
					if (lifespanRange.x > lifespanRange.y)
					{
						lifespanRange.x = lifespanRange.y;
					}
					else if (lifespanRange.x < 0)
					{
						lifespanRange.x = 0;
					}

					if (lifespanRange.y < lifespanRange.x)
					{
						lifespanRange.y = lifespanRange.x;
					}
					else if (lifespanRange.y > MAX_DURATION)
					{
						lifespanRange.y = MAX_DURATION;
					}

					lifespanModif = true;
				}
			}
			else
			{
				if (ImGui::InputFloat("##inputLife", &lifespanRange.x, 1, 5, "%.2f"))
				{
					if (lifespanRange.x > MAX_DURATION)
					{
						lifespanRange.x = MAX_DURATION;
					}
					else if (lifespanRange.x < 0)
					{
						lifespanRange.x = 0;
					}
					lifespanModif = true;
				}
			}
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Random");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##randomLife", &randomLife))
			{
				instance->SetRandomLife(randomLife);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Speed");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (randomSpeed)
			{
				if (ImGui::DragFloat2("##sliderSpeed", &speedRange[0], 1.0f, 0.0f, MAX_SPEED, "%.2f"))
				{
					if (speedRange.x > speedRange.y)
					{
						speedRange.x = speedRange.y;
					}
					else if (speedRange.x < 0)
					{
						speedRange.x = 0;
					}

					if (speedRange.y < speedRange.x)
					{
						speedRange.y = speedRange.x;
					}
					else if (speedRange.y > MAX_SPEED)
					{
						speedRange.y = MAX_SPEED;
					}

					speedModif = true;
				}
			}
			else
			{
				if (ImGui::InputFloat("##inputSpeed", &speedRange.x, 1, 5, "%.2f"))
				{
					if (speedRange.x > MAX_SPEED)
					{
						speedRange.x = MAX_SPEED;
					}
					else if (speedRange.x < 0)
					{
						speedRange.x = 0;
					}
					speedModif = true;
				}
			}
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Random");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##randomSpeed", &randomSpeed))
			{
				instance->SetRandomSpeed(randomSpeed);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Size");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (randomSize)
			{
				if (ImGui::DragFloat2("##sliderSize", &sizeRange[0], 1.0f, 0.0f, MAX_SIZE, "%.2f"))
				{
					if (sizeRange.x > sizeRange.y)
					{
						sizeRange.x = sizeRange.y;
					}
					else if (sizeRange.x < 0)
					{
						sizeRange.x = 0;
					}

					if (sizeRange.y < sizeRange.x)
					{
						sizeRange.y = sizeRange.x;
					}
					else if (sizeRange.y > MAX_SIZE)
					{
						sizeRange.y = MAX_SIZE;
					}

					sizeModif = true;
				}
			}
			else
			{
				if (ImGui::InputFloat("##inputSize", &sizeRange.x, 1, 5, "%.2f"))
				{
					if (sizeRange.x > MAX_SIZE)
					{
						sizeRange.x = MAX_SIZE;
					}
					else if (sizeRange.x < 0)
					{
						sizeRange.x = 0;
					}

					sizeModif = true;
				}
			}
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Random");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##randomSize", &randomSize))
			{
				instance->SetRandomSize(randomSize);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Rotation");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (randomRot)
			{
				if (ImGui::DragFloat2("##sliderRot", &rotRange[0], 1.0f, 0.0f, MAX_ROTATION, "%.2f"))
				{
					if (rotRange.x > rotRange.y)
					{
						rotRange.x = rotRange.y;
					}
					else if (rotRange.x < 0)
					{
						rotRange.x = 0;
					}

					if (rotRange.y < rotRange.x)
					{
						rotRange.y = rotRange.x;
					}
					else if (rotRange.y > MAX_ROTATION)
					{
						rotRange.y = MAX_ROTATION;
					}

					rotModif = true;
				}
			}
			else
			{
				if (ImGui::InputFloat("##inputRot", &rotRange.x, 1, 5, "%.2f"))
				{
					if (rotRange.x > MAX_ROTATION)
					{
						rotRange.x = MAX_ROTATION;
					}
					else if (rotRange.x < 0)
					{
						rotRange.x = 0;
					}

					rotModif = true;
				}
			}
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Random");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##randomRot", &randomRot))
			{
				instance->SetRandomRotation(randomRot);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Gravity");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(165.0f);
			if (randomGrav)
			{
				if (ImGui::DragFloat2("##sliderGrav", &gravRange[0], 1.0f, 0.0f, MAX_GRAVITY, "%.2f"))
				{
					if (gravRange.x > gravRange.y)
					{
						gravRange.x = gravRange.y;
					}
					else if (gravRange.x < 0)
					{
						gravRange.x = 0;
					}

					if (gravRange.y < gravRange.x)
					{
						gravRange.y = gravRange.x;
					}
					else if (gravRange.y > MAX_GRAVITY)
					{
						gravRange.y = MAX_GRAVITY;
					}

					gravModif = true;
				}
			}
			else
			{
				if (ImGui::InputFloat("##inputGrav", &gravRange.x, 1, 5, "%.2f"))
				{
					if (gravRange.x > MAX_GRAVITY)
					{
						gravRange.x = MAX_GRAVITY;
					}
					else if (gravRange.x < 0)
					{
						gravRange.x = 0;
					}

					gravModif = true;
				}
			}
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Text("Random");
			ImGui::SameLine(0.0f, 5.0f);
			if (ImGui::Checkbox("##randomGrav", &randomGrav))
			{
				instance->SetRandomGravity(randomGrav);
			}

			ImGui::TableNextColumn();
			ImGui::Text("Color");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(236.5f);

			if (ImGui::ColorEdit4("##color", &color[0]))
			{
				instance->SetColor(color);
			}

			if (lifespanModif)
			{
				instance->SetLifespanRange(lifespanRange);
			}
			if (speedModif)
			{
				instance->SetSpeedRange(speedRange);
			}
			if (sizeModif)
			{
				instance->SetSizeRange(sizeRange);
			}
			if (rotModif)
			{
				instance->SetRotationRange(rotRange);
			}
			if (gravModif)
			{
				instance->SetGravityRange(gravRange);
			}
			ImGui::EndTable();
		}

		AXO_TODO("Draw Emitter Modules")
		for (ParticleModule* module : instance->GetModules())
		{
			module->DrawImGui();
		}
	}
	else
	{
		AXO_TODO("Select a ParticleEmitter to assign to the EmitterInstance (it acts as a Resource for the instance)")
	}
}
