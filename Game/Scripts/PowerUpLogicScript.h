#pragma once

#include "Scripting\Script.h"
#include "RuntimeInclude.h"

RUNTIME_MODIFIABLE_INCLUDE;

class ComponentTransform;
class ComponentRigidBody;
class PowerUpsManagerScript;

enum class PowerUpType
{
	NONE,
	HEAL,
	ATTACK,
	DEFENSE,
	SPEED
};

class PowerUpLogicScript : public Script
{
public:
	PowerUpLogicScript();
	~PowerUpLogicScript() override = default;

	void Start() override;
	void Update(float deltaTime) override;

	void ActivatePowerUp(GameObject* newParent);

private:
	void RotateMesh(float deltatime);
	void OnCollisionEnter(ComponentRigidBody* other) override;
	void DisablePowerUp() const;

	float timer;
	PowerUpType type;

	ComponentTransform* ownerTransform;
	ComponentRigidBody* ownerRb;
	GameObject* powerUpsManager;
	PowerUpsManagerScript* powerUpManagerScript;

	ComponentTransform* playerTransform;
	GameObject* healPowerUp;
	GameObject* speedPowerUp;
	GameObject* shieldPowerUp;
	GameObject* dmgPowerUp;

	bool isSeeking;
};