#pragma once

#include "Scripting\Script.h"
#include "RuntimeInclude.h"

RUNTIME_MODIFIABLE_INCLUDE;

class ComponentTransform;
class ComponentRigidBody;
class ComponentAnimation;
class ComponentAudioSource;

class FinalBossScript;
class HealthSystem;

enum class AttackState
{
	NONE,
	STARTING_SAFE_JUMP,
	ENDING_SAFE_JUMP,
	EXECUTING_ATTACK,
	STARTING_BACK_JUMP,
	ENDING_BACK_JUMP,
	ON_COOLDOWN
};

class BossMissilesAttackScript : public Script
{
public:
	BossMissilesAttackScript();
	~BossMissilesAttackScript() override = default;

	void Start() override;
	void Update(float deltaTime) override;

	void TriggerMissilesAttack();
	bool CanPerformMissilesAttack() const;

	bool IsAttacking() const;
	void SetIsPaused(bool isPaused);

private:
	void SwapBetweenAttackStates(float deltaTime);

	void MoveUserToPosition(const float3& selectedPosition) const;
	void ManageMissileSpawning(float deltaTime);

	float3 SelectSpawnPosition() const;
	void SpawnMissileInPosition(GameObject* selectedEnemy, const float3& selectedSpawningPosition);

	void RotateToTarget(const float3& targetPosition) const;

	ComponentRigidBody* rigidBody;
	ComponentTransform* transform;
	ComponentAnimation* animator;
	ComponentAudioSource* audioSource;
	FinalBossScript* finalBossScript;
	HealthSystem* healthSystem;

	float3 initialPosition;
	float3 midJumpPositionStart;
	float3 midJumpPositionBack;

	AttackState missilesAttackState;

	float missileAttackDuration;
	float missileAttackCooldown;
	float timeSinceLastMissile;
	float jumpingTimer;

	ComponentTransform* safePositionSelected;
	ComponentTransform* backPositionSelected;

	bool isPaused;

	//Modifiable values
	std::vector<ComponentTransform*> safePositionsTransforms; // Places to where the boss jumps for the attack
	std::vector<ComponentTransform*> backPositionsTransforms; // Places to where the boss gets back after the attack
	ComponentRigidBody* battleArenaAreaSize;

	float missileAttackMaxDuration;
	float missileAttackMaxCooldown;
	float missileMaxSpawnTime;
	float missileSpawningHeight;

	GameObject* missilePrefab;

	float maxJumpingTimer;
};