#pragma once

#include "Scripting\Script.h"
#include "RuntimeInclude.h"
#include "../Scripts/ComboManager.h"

RUNTIME_MODIFIABLE_INCLUDE;

class ModuleInput;
class Scene;

class ComponentAudioSource;
class ComponentTransform;
class ComponentAnimation;

class EntityDetection;
class ComboManager;
class PlayerManagerScript;
class JumpFinisherAttack;
class LightFinisherAttackScript;
class HeavyFinisherAttack;

enum class AttackCombo
{
	IDLE,
	FIRST_ATTACK,
	SECOND_ATTACK,
	THIRD_ATTACK
};

class PlayerAttackScript : public Script
{

public:
	PlayerAttackScript();
	~PlayerAttackScript() override = default;

	bool IsDeathTouched() const;
	void SetIsDeathTouched(bool isDeathTouch);

	bool IsAttackAvailable() const;
	bool IsMelee() const;
	bool IsPerformingJumpAttack() const;

	bool CanAttack() const;
	void SetCanAttack(bool canAttack);

	AttackType GetCurrentAttackType() const;
	bool IsInAttackAnimation() const;
	GameObject* GetEnemyDetected() const;

	void PlayWeaponSounds() const;

private:
	void Start() override;
	void Update(float deltaTime) override;

	void UpdateEnemyDetection();

	void PerformCombos();

	void LightNormalAttack();
	void HeavyNormalAttack();
	void InitJumpAttack();
	void UpdateJumpAttack();
	void EndJumpNormalAttack();
	void EndJumpFinisherAttack();
	void LightFinisher();
	void HeavyFinisher();

	void ResetAttackAnimations(float deltaTime);

	void DamageEnemy(GameObject* enemyAttacked, float damageAttack);
	void ThrowBasicAttack(GameObject* enemyAttacked, float nDamage);
	GameObject* SelectBullet() const;

	bool canAttack;
	bool isAttacking;
	bool isMelee;
	bool isGroundParalyzed;

	float attackCooldown;
	float attackCooldownCounter;
	float comboInitTimer;
	float comboNormalAttackTimer;
	float triggerNextAttackDuration;
	float triggerNextAttackTimer;
	bool isNextAttackTriggered;
	std::string currentAttackAnimation;
	float numAttackComboAnimation;

	float jumpAttackCooldown;
	float timeSinceLastJumpAttack;

	float jumpBeforeJumpAttackCooldown;

	bool isHeavyFinisherReceivedAux;
	bool isHeavyFinisherAvailable;

	bool isDeathTouched;

	ModuleInput* input;

	ComponentAudioSource* audioSource;
	ComponentTransform* transform;
	ComponentAnimation* animation;

	GameObject* animationGO;
	GameObject* enemyDetectionObject;

	EntityDetection* enemyDetection;
	ComboManager* comboSystem;

	float comboCountLight;
	float comboCountHeavy;
	float comboCountJump;

	float attackLightDamage;
	float attackHeavyDamage;
	float bulletVelocity;

	float normalAttackDistance;
	AttackCombo attackComboPhase;
	AttackType currentAttack;
	AttackType lastAttack;

	PlayerManagerScript* playerManager;

	JumpFinisherAttack* jumpFinisherScript;
	LightFinisherAttackScript* lightFinisherScript;
	HeavyFinisherAttack* heavyFinisherAttack;
	GameObject* lightWeapon;
	GameObject* bulletLoader;
	GameObject* pistolGameObject;
	ComponentTransform* bulletInitPosition;

	Scene* loadedScene;
};