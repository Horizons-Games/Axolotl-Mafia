#include "HealthSystem.h"

#include "AxoLog.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentParticleSystem.h"
#include "Components/ComponentAudioSource.h"
#include "Application.h"
#include "ModuleInput.h"
#include "Auxiliar/Audio/AudioData.h"

#include "../Scripts/PlayerAttackScript.h"
#include "../Scripts/EnemyClass.h"
#include "../Scripts/PlayerDeathScript.h"
#include "../Scripts/SpaceshipDeathManager.h"
#include "../Scripts/EnemyDeathScript.h"
#include "../Scripts/PlayerManagerScript.h"
#include "../Scripts/ComboManager.h"
#include "../Scripts/MeshEffect.h"

REGISTERCLASS(HealthSystem);

#define TIME_BETWEEN_EFFECTS 0.10f
#define MAX_TIME_EFFECT_DURATION 0.15f

HealthSystem::HealthSystem() : Script(), currentHealth(100), maxHealth(100), componentAnimation(nullptr), 
	isImmortal(false), enemyParticleSystem(nullptr), attackScript(nullptr),	damageTaken(false), playerManager(nullptr),
	immortalTimer(0.0f), audioSource(nullptr)
{
	REGISTER_FIELD(currentHealth, float);
	REGISTER_FIELD(maxHealth, float);
	REGISTER_FIELD(isImmortal, bool);
	REGISTER_FIELD(enemyParticleSystem, GameObject*);

	REGISTER_FIELD(meshEffect, MeshEffect*);
}

void HealthSystem::Start()
{
	componentAnimation = owner->GetComponent<ComponentAnimation>();
	audioSource = owner->GetComponent<ComponentAudioSource>();
	//componentParticleSystem = enemyParticleSystem->GetComponent<ComponentParticleSystem>();

	//--- This was done because in the gameplay scene there is no particle system
	try
	{
		componentParticleSystem = owner->GetComponent<ComponentParticleSystem>();
	}

	catch (const ComponentNotFoundException&)
	{
		componentParticleSystem = nullptr;
	}
	//---

	// Check that the currentHealth is always less or equal to maxHealth
	if (maxHealth < currentHealth)
	{
		maxHealth = currentHealth;
	}

	meshEffect->FillMeshes(owner);
	meshEffect->ReserveSpace(1);
	meshEffect->AddColor(float4(1.f, 0.f, 0.f, 0.f));

	if (owner->CompareTag("Player"))
	{
		attackScript = owner->GetComponent<PlayerAttackScript>();
		playerManager = owner->GetComponent<PlayerManagerScript>();
	}
}

void HealthSystem::Update(float deltaTime)
{
	meshEffect->DamageEffect();

	if (!EntityIsAlive() && owner->CompareTag("Player") && playerManager->IsParalyzed())
	{
		meshEffect->ClearEffect();
		PlayerDeathScript* playerDeathManager = owner->GetComponent<PlayerDeathScript>();
		playerDeathManager->ManagePlayerDeath();
			
	}
	else if (!EntityIsAlive() && ( owner->CompareTag("Enemy") || owner->CompareTag("PriorityTarget")))
	{
		meshEffect->ClearEffect();
	}

	if (damageTaken)
	{
		componentAnimation->SetParameter("IsTakingDamage", false);
		damageTaken = false;
	}

	if (isImmortal && immortalTimer > 0.0f)
	{
		immortalTimer -= deltaTime;
		if (immortalTimer <= 0.0f)
		{
			isImmortal = false;
		}
	}
}

void HealthSystem::TakeDamage(float damage)
{
	if (!isImmortal) 
	{
		if (owner->CompareTag("Enemy") || owner->CompareTag("PriorityTarget"))
		{
			currentHealth = std::max(currentHealth - damage, 0.0f);
			if (currentHealth == 0 && deathCallback)
			{
				deathCallback();
			}
			else
			{
				componentAnimation->SetParameter("IsTakingDamage", true);
			}
			damageTaken = true;
		}
		else if (owner->CompareTag("Player") && !attackScript->IsPerformingJumpAttack())
		{
			if (attackScript->IsMelee())
			{
				audioSource->PostEvent(AUDIO::SFX::PLAYER::WEAPON::RECEIVEDAMAGE_BIX);
			}
			else
			{
				audioSource->PostEvent(AUDIO::SFX::PLAYER::WEAPON::RECEIVEDAMAGE_ALLURA);
			}
			float playerDefense = owner->GetComponent<PlayerManagerScript>()->GetPlayerDefense();
			float actualDamage = std::max(damage - playerDefense, 0.f);

			currentHealth -= actualDamage;

			if (currentHealth <= 0)
			{
				playerManager->ParalyzePlayer(true);
				owner->GetComponent<ComboManager>()->SuccessfulAttack(-100.f, AttackType::NONE);
			}
			else
			{
				componentAnimation->SetParameter("IsTakingDamage", true);
				damageTaken = true;
				ModuleInput* input = App->GetModule<ModuleInput>();
				input->Rumble();
			}
		}
		else if (owner->CompareTag("PlayerSpaceship"))
		{
			currentHealth -= damage;
			SetImmortalTimer(1.0f);
			if (currentHealth <= 0.0f)
			{
				SpaceshipDeathManager* spaceshipDeathManager = owner->GetComponent<SpaceshipDeathManager>();
				spaceshipDeathManager->ManageSpaceshipDeath();
			}
		}

		if (EntityIsAlive())
		{
			meshEffect->StartEffect(MAX_TIME_EFFECT_DURATION, TIME_BETWEEN_EFFECTS);
		}

		if (componentParticleSystem)
		{
			componentParticleSystem->Play();
		}

		// componentParticleSystem->Pause();
	}
}

void HealthSystem::HealLife(float amountHealed)
{
	currentHealth = std::min(currentHealth + amountHealed, maxHealth);
}

bool HealthSystem::EntityIsAlive() const
{
	return currentHealth > 0.0f || isImmortal;
}

float HealthSystem::GetMaxHealth() const
{
	return maxHealth;
}

void HealthSystem::SetMaxHealth(float maxHealth)
{
	this->maxHealth = maxHealth;
}

bool HealthSystem::IsImmortal() const
{
	return isImmortal;
}

void HealthSystem::SetIsImmortal(bool isImmortal)
{
	this->isImmortal = isImmortal;
}

void HealthSystem::SetImmortalTimer(float duration)
{
	isImmortal = true;
	immortalTimer = duration;
}

void HealthSystem::SetDeathCallback(std::function<void(void)>&& callDeath)
{
	deathCallback = std::move(callDeath);
}

float HealthSystem::GetCurrentHealth() const
{
	return currentHealth;
}

MeshEffect* HealthSystem::GetMeshEffect() const
{
	return meshEffect;
}