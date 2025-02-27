#include "StdAfx.h"
#include "BossShieldAttackScript.h"

#include "Application.h"
#include "Modules/ModuleRandom.h"
//#include "Modules/ModuleScene.h"
//#include "Scene/Scene.h"

#include "Auxiliar/Audio/AudioData.h"

#include "Components/ComponentScript.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentRigidBody.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentAudioSource.h"

#include "../Scripts/EnemyClass.h"
#include "../Scripts/BossShieldScript.h"
#include "../Scripts/BossShieldEnemiesSpawner.h"
#include "../Scripts/EnemyDroneScript.h"
#include "../Scripts/EnemyVenomiteScript.h"
#include "../Scripts/HealthSystem.h"
#include "../Scripts/PathBehaviourScript.h"
#include "../Scripts/AIMovement.h"

REGISTERCLASS(BossShieldAttackScript);

BossShieldAttackScript::BossShieldAttackScript() : Script(), bossShieldObject(nullptr), isShielding(false),
	shieldingTime(0.0f), shieldingMaxTime(20.0f), triggerShieldAttackCooldown(false), shieldAttackCooldown(0.0f),
	shieldAttackMaxCooldown(50.0f), triggerEnemySpawning(false), enemiesToSpawnParent(nullptr),
	enemySpawnTime(0.0f), enemyMaxSpawnTime(2.0f), battleArenaAreaSize(nullptr), healthSystemScript(nullptr),
	currentPath(0), animator(nullptr), audioSource(nullptr), needsToSyncAnims(true)
{
	REGISTER_FIELD(shieldingMaxTime, float);
	REGISTER_FIELD(shieldAttackMaxCooldown, float);

	REGISTER_FIELD(bossShieldObject, BossShieldScript*);

	REGISTER_FIELD(enemyMaxSpawnTime, float);

	REGISTER_FIELD(enemiesToSpawnParent, GameObject*);

	REGISTER_FIELD(battleArenaAreaSize, ComponentRigidBody*);

	REGISTER_FIELD(initsPaths, std::vector<GameObject*>);

	REGISTER_FIELD(manageEnemySpawner, bool);
}

void BossShieldAttackScript::Init()
{
	Assert(enemiesToSpawnParent != nullptr,
		axo::Format("No spawner of enemies set for the Boss Shield Attack!! Owner: {}", GetOwner()));
}

void BossShieldAttackScript::Start()
{
	shieldingTime = shieldingMaxTime;

	enemiesReadyToSpawn.reserve(enemiesToSpawnParent->GetChildren().size());
	enemiesNotReadyToSpawn.reserve(enemiesToSpawnParent->GetChildren().size());

	for (GameObject* enemyToSpawn : enemiesToSpawnParent->GetChildren())
	{
		enemiesReadyToSpawn.push_back(enemyToSpawn);
	}

	healthSystemScript = owner->GetComponent<HealthSystem>();

	if (!manageEnemySpawner)
	{
		bossShieldEnemiesSpawner = owner->GetComponent<BossShieldEnemiesSpawner>();
	}

	animator = owner->GetComponent<ComponentAnimation>();
	audioSource = owner->GetComponent<ComponentAudioSource>();
}

void BossShieldAttackScript::Update(float deltaTime)
{
	if (isPaused)
	{
		return;
	}

	if (bossShieldObject->WasHitBySpecialTarget())
	{
		shieldingTime = 0.0f;
		bossShieldObject->DisableHitBySpecialTarget();
		animator->SetParameter("IsShieldAttack", false);
		animator->SetParameter("IsInvoking", false);
	}

	ManageShield(deltaTime);
	ManageEnemiesSpawning(deltaTime);

	ManageRespawnOfEnemies();

	if (animator->GetActualStateName() == "BossShieldInvokeEnemy")
	{
		animator->SetParameter("IsInvoking", false);
	}
}

void BossShieldAttackScript::TriggerShieldAttack(bool needsToSyncAnims)
{
	LOG_INFO("The shield attack was triggered");

	healthSystemScript->SetIsImmortal(true);
	//bossShieldEnemiesSpawner->StartSpawner();
	owner->GetComponent< AIMovement>()->SetMovementStatuses(false, false);

	isShielding = true;
	animator->SetParameter("IsShieldAttack", true);
	shieldAttackCooldown = shieldAttackMaxCooldown;

	audioSource->PostEvent(AUDIO::SFX::NPC::FINALBOSS::ENERGYSHIELD);

	triggerEnemySpawning = true;
	this->needsToSyncAnims = needsToSyncAnims;
}

bool BossShieldAttackScript::CanPerformShieldAttack() const
{
	return shieldAttackCooldown <= 0.0f && !IsAttacking();
}

bool BossShieldAttackScript::IsAttacking() const
{
	return isShielding;
}

void BossShieldAttackScript::ManageShield(float deltaTime)
{
	if (isShielding)
	{
		bool isFinalBossShieldAnimReady = animator->GetActualStateName() == "BossShieldIdle" ||
			animator->GetActualStateName() == "BossShieldInvokeEnemy";

		if ((isFinalBossShieldAnimReady || !needsToSyncAnims) // Miniboss does not need to wait to sync animations
			&& !bossShieldObject->GetOwner()->IsEnabled())
		{
			bossShieldObject->ActivateShield();

		}

		shieldingTime -= deltaTime;
		healthSystemScript->HealLife(deltaTime * 3);

		if (shieldingTime <= 0.0f)
		{
			DisableShielding();
		}
	}
	else if (triggerShieldAttackCooldown)
	{
		shieldAttackCooldown -= deltaTime;
		if (shieldAttackCooldown <= 0.0f)
		{
			triggerShieldAttackCooldown = false;
		}
	}
}

void BossShieldAttackScript::ManageEnemiesSpawning(float deltaTime)
{
	if (!triggerEnemySpawning)
	{
		return;
	}

	// The final boss has a script that manages the spawning
	if (!manageEnemySpawner)
	{
		return;
	}

	if (enemySpawnTime > 0.0f)
	{
		enemySpawnTime -= deltaTime;
	}
	else
	{
		float3 selectedPosition = SelectSpawnPosition();
		GameObject* selectedEnemy = SelectEnemyToSpawn();

		if (selectedEnemy != nullptr)
		{
			SpawnEnemyInPosition(selectedEnemy, selectedPosition);
		}
		else
		{
			LOG_INFO("No enemy available to spawn");
		}

		enemySpawnTime = enemyMaxSpawnTime;
	}
}

void BossShieldAttackScript::ManageRespawnOfEnemies()
{
	for (int i = 0; i < enemiesNotReadyToSpawn.size(); ++i)
	{
		GameObject* enemy = enemiesNotReadyToSpawn[i];
		if (enemy->IsEnabled())
		{
			continue;
		}

		EnemyClass* enemyClass = enemy->GetComponent<EnemyClass>();
		enemyClass->ActivateNeedsToBeReset();

		enemiesReadyToSpawn.push_back(enemy);
		enemiesNotReadyToSpawn.erase(enemiesNotReadyToSpawn.begin() + i);
		--i;
	}
}

void BossShieldAttackScript::DisableShielding()
{
	isShielding = false;
	animator->SetParameter("IsShieldAttack", false);
	animator->SetParameter("IsInvoking", false);
	shieldingTime = shieldingMaxTime;

	bossShieldObject->DeactivateShield();

	healthSystemScript->SetIsImmortal(false);
	owner->GetComponent< AIMovement>()->SetMovementStatuses(true, true);
	if (!manageEnemySpawner)
	{
		bossShieldEnemiesSpawner->StopSpawner();
	}

	audioSource->PostEvent(AUDIO::SFX::NPC::FINALBOSS::ENERGYSHIELD_STOP);

	triggerShieldAttackCooldown = true;
	triggerEnemySpawning = false;
}

GameObject* BossShieldAttackScript::SelectEnemyToSpawn()
{
	if (enemiesReadyToSpawn.empty())
	{
		return nullptr;
	}

	int enemyRange = static_cast<int>(enemiesReadyToSpawn.size() - 1);
	int randomEnemyIndex = App->GetModule<ModuleRandom>()->RandomNumberInRange(enemyRange);
	GameObject* selectedEnemy = enemiesReadyToSpawn.at(randomEnemyIndex);

	EnemyClass* enemyClass = selectedEnemy->GetComponent<EnemyClass>();

	if (!initsPaths.empty())
	{
		selectedEnemy->GetComponent<PathBehaviourScript>()->SetNewPath(initsPaths[currentPath]);
		currentPath = (currentPath + 1) % initsPaths.size();
	}

	if (enemyClass->NeedsToBeReset())
	{
		if (enemyClass->GetEnemyType() == EnemyTypes::DRONE)
		{
			selectedEnemy->GetComponent<EnemyDroneScript>()->ResetValues();
		}
		else if (enemyClass->GetEnemyType() == EnemyTypes::VENOMITE)
		{
			selectedEnemy->GetComponent<EnemyVenomiteScript>()->ResetValues();
		}

		enemyClass->DeactivateNeedsToBeReset();
	}

	enemiesReadyToSpawn.erase(enemiesReadyToSpawn.begin() + randomEnemyIndex);

	enemiesNotReadyToSpawn.push_back(selectedEnemy);

	return selectedEnemy;
}

float3 BossShieldAttackScript::SelectSpawnPosition() const
{
	float areaRadius = battleArenaAreaSize->GetRadius();
	float areaDiameter = areaRadius * 2.0f;

	float randomXPos = (App->GetModule<ModuleRandom>()->RandomNumberInRange(areaDiameter) - areaRadius)
		+ (App->GetModule<ModuleRandom>()->RandomNumberInRange(100.0f) * 0.01f);
	float randomZPos = (App->GetModule<ModuleRandom>()->RandomNumberInRange(areaDiameter) - areaRadius)
		+ (App->GetModule<ModuleRandom>()->RandomNumberInRange(100.0f) * 0.01f);
	float3 selectedSpawningPosition =
		float3(randomXPos,
			enemiesToSpawnParent->GetComponent<ComponentTransform>()->GetGlobalPosition().y,
			randomZPos);
	selectedSpawningPosition += battleArenaAreaSize->GetOwner()->GetComponent<ComponentTransform>()->GetGlobalPosition();

	return selectedSpawningPosition;
}

void BossShieldAttackScript::SpawnEnemyInPosition(GameObject* selectedEnemy, const float3& selectedSpawningPosition)
{
	ComponentTransform* selectedEnemyTransform = selectedEnemy->GetComponent<ComponentTransform>();
	selectedEnemyTransform->SetGlobalPosition(float3(selectedSpawningPosition.x,
		selectedSpawningPosition.y,
		selectedSpawningPosition.z));
	selectedEnemyTransform->RecalculateLocalMatrix();

	ComponentRigidBody* selectedEnemyRigidBody = selectedEnemy->GetComponent<ComponentRigidBody>();
	selectedEnemyRigidBody->UpdateRigidBody();

	selectedEnemy->Enable();

	// ** UNUSABLE FOR NOW **
	/*
	GameObject* newEnemy = App->GetModule<ModuleScene>()->GetLoadedScene()->
		DuplicateGameObject(selectedEnemy->GetName(), selectedEnemy, selectedEnemy->GetParent());
	newEnemy->Enable();

	ComponentTransform* newEnemyTransform = newEnemy->GetComponent<ComponentTransform>();
	newEnemyTransform->SetGlobalPosition(selectedSpawningPosition);
	newEnemyTransform->RecalculateLocalMatrix();

	ComponentRigidBody* newEnemyRigidBody = newEnemy->GetComponent<ComponentRigidBody>();
	newEnemyRigidBody->SetDefaultPosition();
	newEnemyRigidBody->Enable();
	*/
	// ** UNUSABLE FOR NOW **
}

void BossShieldAttackScript::SetIsPaused(bool isPaused)
{
	this->isPaused = isPaused;
	bossShieldEnemiesSpawner->SetIsPaused(isPaused);
}