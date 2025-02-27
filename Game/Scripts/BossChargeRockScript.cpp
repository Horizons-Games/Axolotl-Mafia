#include "StdAfx.h"
#include "BossChargeRockScript.h"

#include "Application.h"
#include "Modules/ModuleScene.h"
#include "Scene/Scene.h"
#include "Auxiliar/Audio/AudioData.h"

#include "Components/ComponentScript.h"
#include "Components/ComponentRigidbody.h"
#include "Components/ComponentBreakable.h"
#include "Components/ComponentObstacle.h"
#include "Components/ComponentAudioSource.h"
#include "Components/ComponentParticleSystem.h"

#include "../Scripts/HealthSystem.h"
#include "../Scripts/WaypointStateScript.h"

REGISTERCLASS(BossChargeRockScript);

BossChargeRockScript::BossChargeRockScript() : Script(), rockState(RockStates::SKY), fallingRockDamage(10.0f),
	despawnTimer(0.0f), despawnMaxTimer(30.0f), fallingDespawnMaxTimer(30.0f),fallingTimer(0.0f),
	breakTimer(0.0f),breakMaxTimer(30.0f), triggerRockDespawn(false),
	rockHitAndRemained(false), waypointCovered(nullptr), audioSource(nullptr)
{
	REGISTER_FIELD(fallingRockDamage, float);
	REGISTER_FIELD(despawnMaxTimer, float);
	REGISTER_FIELD(fallingDespawnMaxTimer, float);
	REGISTER_FIELD(breakMaxTimer, float);
}

void BossChargeRockScript::Start()
{
	despawnTimer = despawnMaxTimer;
	breakTimer = breakMaxTimer;
	fallingTimer = fallingDespawnMaxTimer;

	meshEffect = owner->GetComponent<MeshEffect>();
	meshEffect->FillMeshes(owner);
	meshEffect->AddColor(float4(0.f, 0.f, 0.f, 1.f));
	//breakRockVFX = owner->GetComponent<ComponentParticleSystem>();
	rigidBody = owner->GetComponent<ComponentRigidBody>();
	audioSource = owner->GetComponent<ComponentAudioSource>();
	rockGravity = rigidBody->GetRigidBody()->getGravity();
	preAreaEffectVFX = owner->GetChildren()[1]->GetComponent<ComponentParticleSystem>();

	preAreaEffectVFX->Enable();
	preAreaEffectVFX->Play();
}

void BossChargeRockScript::Update(float deltaTime)
{
	if (isPaused)
	{
		rigidBody->GetRigidBody()->setGravity(btVector3(0, 0, 0));
		rigidBody->GetRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
		return;
	}

	if (rockState != RockStates::FLOOR)
	{
		rigidBody->GetRigidBody()->setGravity(rockGravity);
	}

	if (triggerRockDespawn)
	{
		despawnTimer -= deltaTime;
		meshEffect->FadeEffect();
		if (despawnTimer <= 0.0f)
		{
			//meshEffect->ClearEffect();
			DestroyRock();
		}
	}
	if (triggerRockDespawnbyFalling)
	{
		meshEffect->FadeEffect();
		fallingTimer -= deltaTime;
		if (fallingTimer <= 0.0f)
		{
			//meshEffect->ClearEffect();
			DestroyRock();
		}
	}
	if (triggerBreakTimer)
	{
		breakTimer -= deltaTime;
		if (breakTimer <= 0.0f)
		{
			meshEffect->StartEffect(despawnMaxTimer-breakMaxTimer, 0);
			owner->GetComponent<ComponentBreakable>()->BreakComponent();
			/*breakRockVFX->Stop();
			breakRockVFX->Disable();*/
		}
	}
}

void BossChargeRockScript::OnCollisionEnter(ComponentRigidBody* other)
{
	if (rockState == RockStates::SKY && other->GetOwner()->CompareTag("Rock"))
	{
		BossChargeRockScript* otherRock = other->GetOwner()->GetComponent<BossChargeRockScript>();

		if (!otherRock->WasRockHitAndRemained())
		{
			other->GetOwner()->GetComponent<BossChargeRockScript>()->DeactivateRock();
			rockHitAndRemained = true;
		}
		else if (otherRock->WasRockHitAndRemained() || otherRock->GetRockState() != RockStates::SKY)
		{
			DeactivateRock();
			preAreaEffectVFX->Stop();
		}

		LOG_DEBUG("Rock deactivated");
	}
	else if (rockState == RockStates::FALLING)
	{
		if (other->GetOwner()->CompareTag("Enemy") || other->GetOwner()->CompareTag("PriorityTarget") || other->GetOwner()->CompareTag("Player"))
		{
			other->GetOwner()->GetComponent<HealthSystem>()->TakeDamage(fallingRockDamage);
			rockState = RockStates::HIT_ENEMY;
			triggerRockDespawnbyFalling = true;
			owner->GetComponent<ComponentBreakable>()->BreakComponentFalling();
			meshEffect->StartEffect(fallingTimer*2.5f,0.f);
			
			audioSource->PostEvent(AUDIO::SFX::NPC::FINALBOSS::CHARGE_ROCKS_IMPACT);
			// VFX Here: Rock hit an enemy on the head while falling
			preAreaEffectVFX->Stop();
		}
		else if (other->GetOwner()->CompareTag("Waypoint"))
		{
			waypointCovered = other->GetOwner()->GetComponent<WaypointStateScript>();
			waypointCovered->SetWaypointState(WaypointStates::UNAVAILABLE);
		}
		else if (other->GetOwner()->CompareTag("Floor"))
		{
			owner->GetComponent<ComponentObstacle>()->AddObstacle();
			triggerBreakTimer = true;
			/*breakRockVFX->Enable();
			breakRockVFX->Play();*/
			rockState = RockStates::FLOOR;

			audioSource->PostEvent(AUDIO::SFX::NPC::FINALBOSS::CHARGE_ROCKS_IMPACT);
			preAreaEffectVFX->Stop();
			// VFX Here: Rock hit the floor
		}
	}
	else
	{
		triggerRockDespawn = true;
	}	
}

void BossChargeRockScript::SetRockState(RockStates newState)
{
	rockState = newState;
}

RockStates BossChargeRockScript::GetRockState() const
{
	return rockState;
}

void BossChargeRockScript::DeactivateRock()
{
	if (rockState == RockStates::HIT_ENEMY)
	{
		// Only disable the root node of the rock and the rigid so the particles can still be seen
		owner->GetComponent<ComponentRigidBody>()->Disable();
		if (!owner->GetChildren().empty())
		{
			owner->GetChildren().front()->Disable();
		}

		// VFX Here: Disappear/Break rock (particles in the parent will still play, only the fbx will disappear)
	}
	else
	{
		owner->Disable();
	}

	if (waypointCovered)
	{
		waypointCovered->SetWaypointState(WaypointStates::AVAILABLE);
	}

	triggerRockDespawn = true;
}

void BossChargeRockScript::DestroyRock() const
{
	App->GetModule<ModuleScene>()->GetLoadedScene()->DestroyGameObject(owner);
}

void BossChargeRockScript::SetPauseRock(bool isPaused)
{
	this->isPaused = isPaused;
	if (preAreaEffectVFX) 
	{
		preAreaEffectVFX->Pause();
	}
}

bool BossChargeRockScript::WasRockHitAndRemained() const
{
	return rockHitAndRemained;
}