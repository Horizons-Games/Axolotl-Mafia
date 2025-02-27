#include "StdAfx.h"
#include "PlayerHackingUseScript.h"

#include "HackZoneScript.h"
#include "UIHackingManager.h"
#include "SwitchPlayerManagerScript.h"
#include "PlayerManagerScript.h"
#include "PlayerJumpScript.h"
#include "PlayerMoveScript.h"
#include "PlayerAttackScript.h"

#include "Application.h"
#include "ModuleInput.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentRigidBody.h"
#include "Components/ComponentParticleSystem.h"
#include "Components/ComponentAudioSource.h"
#include "Physics/Physics.h"
#include "Auxiliar/Audio/AudioData.h"

#include "MathGeoLib/Include/Geometry/Ray.h"

REGISTERCLASS(PlayerHackingUseScript);

PlayerHackingUseScript::PlayerHackingUseScript()
	: Script(), isHackingActive(false), hackingTag("Hackeable"), isHackingButtonPressed(false), hackZone(nullptr),
	audioSource(nullptr)
{
	REGISTER_FIELD(hackingManager, UIHackingManager*);
	REGISTER_FIELD(switchPlayerManager, SwitchPlayerManagerScript*);
}

void PlayerHackingUseScript::Start()
{
	input = App->GetModule<ModuleInput>();
	transform = GetOwner()->GetComponent<ComponentTransform>();
	rigidBody = GetOwner()->GetComponent<ComponentRigidBody>();
	audioSource = GetOwner()->GetComponent<ComponentAudioSource>();
	playerManager = GetOwner()->GetComponent<PlayerManagerScript>();
}

void PlayerHackingUseScript::Update(float deltaTime)
{
	if (playerManager->IsPaused())
	{
		return;
	}
	currentTime += deltaTime;
	hackingManager->SetHackingTimerValue(maxHackTime, currentTime);

	PlayerActions currentAction = playerManager->GetPlayerState();
	bool isJumping = currentAction == PlayerActions::JUMPING ||
		currentAction == PlayerActions::DOUBLEJUMPING;

	bool isAttacking = playerManager->GetAttackManager()->IsInAttackAnimation();
	FindHackZone(hackingTag);
	CheckCurrentHackZone();

	if (input->GetKey(SDL_SCANCODE_E) == KeyState::DOWN && !isHackingActive &&
		!isJumping && !isAttacking && !playerManager->GetAttackManager()->IsMelee())
	{
		if (hackZone && !hackZone->IsCompleted() && !playerManager->IsParalyzed())
		{
			InitHack();
			isHackingButtonPressed = true;
		}
	}

	if (isHackingActive)
	{
		if (input->GetKey(SDL_SCANCODE_E) == KeyState::UP)
		{
			isHackingButtonPressed = false;
		}

		if (input->GetKey(SDL_SCANCODE_E) == KeyState::DOWN && !isHackingButtonPressed)
		{
			FinishHack();
		}

		if (currentTime > maxHackTime)
		{
			RestartHack();
		}
		else
		{
			SDL_Scancode key;
			SDL_GameControllerButton button;
			for (auto command : HackingCommand::allHackingCommands)
			{
				auto keyButtonPair = HackingCommand::FromCommand(command);
				key = keyButtonPair->first;
				button = keyButtonPair->second;

				if (input->GetKey(key) == KeyState::DOWN || input->GetGamepadButton(button) == KeyState::DOWN)
				{
					userCommandInputs.push_back(command);
					LOG_DEBUG("User add key/button to combination");
					audioSource->PostEvent(AUDIO::SFX::UI::HACKING::CORRECT);

					hackingManager->RemoveInputVisuals();
					break;
				}
			}

			bool isMismatch = false;
			for (int i = 0; i < userCommandInputs.size(); ++i)
			{
				if (userCommandInputs[i] != commandCombination[i])
				{
					isMismatch = true;
					break;
				}
			}

			if (isMismatch)
			{
				LOG_DEBUG("Mismatch detected. Hacking will fail.");
				RestartHack();

				audioSource->PostEvent(AUDIO::SFX::UI::HACKING::WRONG);
			}

			if (userCommandInputs == commandCombination)
			{
				LOG_DEBUG("Hacking completed");
				audioSource->PostEvent(AUDIO::SFX::UI::HACKING::FINISHED);
				App->GetModule<ModuleAudio>()->SetLowPassFilter(0.0f);
				hackZone->SetCompleted();
				FinishHack();
			}

		}
	}
}

bool PlayerHackingUseScript::IsInsideValidHackingZone() const
{
	return hackZone && !hackZone->IsCompleted();
}

// DEBUG METHOD
void PlayerHackingUseScript::PrintCombination()
{
	std::string combination;
	for (auto element :commandCombination)
	{
		char c = '\0';
		switch (element) {
		case COMMAND_A: 
			c = '_'; 
			break;
		case COMMAND_X:
			c = 'R'; 
			break;
		case COMMAND_Y:
			c = 'T';
			break;
		default: 
			break;
		}

		combination += c;
	}

	LOG_DEBUG(combination);
}

void PlayerHackingUseScript::InitHack()
{
	DisableAllInteractions();
	isHackingActive = true;
	currentTime = App->GetDeltaTime();
	maxHackTime = hackZone->GetMaxTime();
	hackZone->GenerateCombination();
	switchPlayerManager->SetIsSwitchAvailable(false);

	userCommandInputs.reserve(static_cast<size_t>(hackZone->GetSequenceSize()));
	hackZone->GetOwner()->GetChildren()[0]->GetComponent<ComponentParticleSystem>()->Stop();

	commandCombination = hackZone->GetCommandCombination();
	for (auto command : commandCombination)
	{
		hackingManager->AddInputVisuals(command);
	}

	hackingManager->EnableHackingTimer();

	App->GetModule<ModuleAudio>()->SetLowPassFilter(60.0f);
	// PrintCombination();
	LOG_DEBUG("Hacking is active");
}

void PlayerHackingUseScript::FinishHack()
{
	EnableAllInteractions();
	isHackingActive = false;
	hackZone = nullptr;
	switchPlayerManager->SetIsSwitchAvailable(true);

	userCommandInputs.clear();

	hackingManager->CleanInputVisuals();
	hackingManager->DisableHackingTimer();

	LOG_DEBUG("Hacking is finished");
}

void PlayerHackingUseScript::RestartHack()
{
	userCommandInputs.clear();
	hackingManager->CleanInputVisuals();
	hackingManager->DisableHackingTimer();

	currentTime = App->GetDeltaTime();
	maxHackTime = hackZone->GetMaxTime();
	hackZone->GenerateCombination();

	userCommandInputs.reserve(static_cast<size_t>(hackZone->GetSequenceSize()));

	commandCombination = hackZone->GetCommandCombination();
	for (auto command : commandCombination)
	{
		hackingManager->AddInputVisuals(command);
	}

	hackingManager->EnableHackingTimer();

	// PrintCombination();
	input->Rumble();
	LOG_DEBUG("Hacking is restarted");
}

void PlayerHackingUseScript::DisableAllInteractions() const
{
	playerManager->SetPlayerState(PlayerActions::IDLE);
	playerManager->PausePlayer(true);
}

void PlayerHackingUseScript::EnableAllInteractions() const
{
	playerManager->SetPlayerState(PlayerActions::IDLE);
	playerManager->PausePlayer(false);
}

void PlayerHackingUseScript::FindHackZone(const std::string& tag)
{
	RaycastHit hit;
	btVector3 rigidBodyOrigin = rigidBody->GetRigidBodyOrigin();
	float3 origin = float3(rigidBodyOrigin.getX(), rigidBodyOrigin.getY(), rigidBodyOrigin.getZ());
	int raytries = 0;

	while (!hackZone && raytries < 4)
	{
		Ray ray(origin + float3(0.f, static_cast<float>(1 * raytries), 0.f), transform->GetGlobalForward());
		LineSegment line(ray, 300);
		raytries++;

		if (Physics::RaycastToTag(line, hit, owner, tag))
		{
			GameObject* hackZoneObject = hit.gameObject;
			HackZoneScript* hackZoneScript = hackZoneObject->GetComponent<HackZoneScript>();
			ComponentTransform* hackZoneTransform = hackZoneObject->GetComponent<ComponentTransform>();

			float3 hackZonePosition = hackZoneTransform->GetGlobalPosition();
			float3 playerPosition = transform->GetGlobalPosition();

			float distance = (playerPosition - hackZonePosition).Length();

			if (distance < hackZoneScript->GetInfluenceRadius() && !hackZoneScript->IsCompleted())
			{
				hackZone = hackZoneScript;
				if (playerManager->GetAttackManager()->IsMelee())
				{	
					hackZone->GetOwner()->GetChildren()[0]->GetComponent<ComponentParticleSystem>()->Stop();
					hackZone->GetOwner()->GetChildren()[1]->GetComponent<ComponentParticleSystem>()->Play();
					hackZone->GetOwner()->GetChildren()[2]->GetComponent<ComponentParticleSystem>()->Play();
				}
				else
				{
					hackZone->GetOwner()->GetChildren()[0]->GetComponent<ComponentParticleSystem>()->Play();
					hackZone->GetOwner()->GetChildren()[1]->GetComponent<ComponentParticleSystem>()->Stop();
					hackZone->GetOwner()->GetChildren()[2]->GetComponent<ComponentParticleSystem>()->Stop();
				}
			}
		}
	}
}

void PlayerHackingUseScript::StopHackingParticles()
{
	if (hackZone)
	{
		hackZone->GetOwner()->GetChildren()[0]->GetComponent<ComponentParticleSystem>()->Stop();
		hackZone->GetOwner()->GetChildren()[1]->GetComponent<ComponentParticleSystem>()->Stop();
		hackZone->GetOwner()->GetChildren()[2]->GetComponent<ComponentParticleSystem>()->Stop();
		hackZone = nullptr;
	}
}

void PlayerHackingUseScript::CheckCurrentHackZone()
{
	if (hackZone)
	{
		float3 hackZonePosition = hackZone->GetOwner()->GetComponent<ComponentTransform>()->GetGlobalPosition();
		float3 playerPosition = transform->GetGlobalPosition();
		float distance = (playerPosition - hackZonePosition).Length();

		if (distance > hackZone->GetInfluenceRadius())
		{
			StopHackingParticles();
		}
	}
}

bool PlayerHackingUseScript::IsHackingActive() const
{
	return isHackingActive;
}
