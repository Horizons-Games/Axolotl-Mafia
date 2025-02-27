#include "PlayerMoveScript.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModulePlayer.h"
#include "Camera/Camera.h"
#include "Geometry/Frustum.h"

#include "Components/ComponentRigidBody.h"
#include "Components/ComponentTransform.h"
#include "Components/ComponentAudioSource.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentParticleSystem.h"

#include "Auxiliar/Audio/AudioData.h"

#include "../Scripts/PlayerJumpScript.h"
#include "../Scripts/PlayerAttackScript.h"
#include "../Scripts/PlayerManagerScript.h"
#include "../Scripts/PlayerForceUseScript.h"

#include "GameObject/GameObject.h"
#include "AxoLog.h"

REGISTERCLASS(PlayerMoveScript);

PlayerMoveScript::PlayerMoveScript() : Script(), componentTransform(nullptr),
componentAudio(nullptr), componentAnimation(nullptr), dashForce(30.0f), 
playerManager(nullptr), isParalyzed(false), desiredRotation(float3::zero), waterCounter(0),
lightAttacksMoveFactor(2.0f), heavyAttacksMoveFactor(3.0f), dashRollTime(0.0f), 
dashRollCooldown(0.1f), timeSinceLastDash(0.0f), dashRollDuration(0.2f), totalDirection(float3::zero),
isTriggeringStoredDash(false), rotationAttackVelocity(100.0f), dashBix(nullptr), ghostBixDashing(false), rollAllura(nullptr)
{
	REGISTER_FIELD(dashForce, float);
	REGISTER_FIELD(isParalyzed, bool);
	REGISTER_FIELD(lightAttacksMoveFactor, float);
	REGISTER_FIELD(heavyAttacksMoveFactor, float);
	REGISTER_FIELD(rotationAttackVelocity, float);
	REGISTER_FIELD(dashRollCooldown, float);
	REGISTER_FIELD(dashRollDuration, float);
	REGISTER_FIELD(dashBix, GameObject*);
	REGISTER_FIELD(rollAllura, GameObject*);
	
}

void PlayerMoveScript::Start()
{
	componentTransform = owner->GetComponent<ComponentTransform>();
	componentAudio = owner->GetComponent<ComponentAudioSource>();
	componentAnimation = owner->GetComponent<ComponentAnimation>();
	playerManager = owner->GetComponent<PlayerManagerScript>();

	if (owner->HasComponent<PlayerForceUseScript>())
	{
		forceScript = owner->GetComponent<PlayerForceUseScript>();
	}
	
	rigidBody = owner->GetComponent<ComponentRigidBody>();
	jumpScript = owner->GetComponent<PlayerJumpScript>();
	playerAttackScript = owner->GetComponent<PlayerAttackScript>();
	btRigidbody = rigidBody->GetRigidBody();

	camera = App->GetModule<ModulePlayer>()->GetCameraPlayer();
	input = App->GetModule<ModuleInput>();

	cameraFrustum = *camera->GetFrustum();

	previousMovements = 0;
	currentMovements = 0;

	desiredRotation = componentTransform->GetGlobalForward();
}

void PlayerMoveScript::PreUpdate(float deltaTime)
{
	if (!playerAttackScript->IsPerformingJumpAttack())
	{
		if (forceScript && forceScript->IsForceActive())
		{
			return;
		}
		else
		{
			btRigidbody->setAngularVelocity({ 0.0f,0.0f,0.0f });
		}

		if (isParalyzed)
		{
			componentAnimation->SetParameter("IsRunning", false);
			componentAnimation->SetParameter("IsDashing", false);
			rigidBody->GetRigidBody()->setLinearVelocity(btVector3(0.f,
				rigidBody->GetRigidBody()->getLinearVelocity().getY(), 0.f));
			componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
			return;
		}

		Move(deltaTime);
		MoveRotate(deltaTime);
		DashRoll(deltaTime);
	}
}

void PlayerMoveScript::Move(float deltaTime)
{
	desiredRotation = owner->GetComponent<ComponentTransform>()->GetGlobalForward();

	btRigidbody->setAngularFactor(btVector3(0.0f, 0.0f, 0.0f));
	btRigidbody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));

	btVector3 movement(0, 0, 0);

	float newSpeed = playerManager->GetPlayerSpeed();

	JoystickHorizontalDirection horizontalDirection = input->GetLeftJoystickDirection().horizontalDirection;
	JoystickVerticalDirection verticalDirection = input->GetLeftJoystickDirection().verticalDirection;

	previousMovements = currentMovements;
	currentMovements = 0;

	if (input->GetCurrentInputMethod() == InputMethod::GAMEPAD &&
		(horizontalDirection != JoystickHorizontalDirection::NONE ||
			verticalDirection != JoystickVerticalDirection::NONE))
	{
		cameraFrustum = *camera->GetFrustum();
		float3 front =
			float3(cameraFrustum.Front().Normalized().x, 0, cameraFrustum.Front().Normalized().z);

		float3 joystickDirection = float3(input->GetLeftJoystickMovement().horizontalMovement, 0.0f, input->GetLeftJoystickMovement().verticalMovement).Normalized();

		float angle = math::Acos(joystickDirection.Dot(float3(0, 0, -1)));

		if (joystickDirection.x < 0)
		{
			angle = -angle;
		}

		float x, z;
		x = (front.x * math::Cos(angle)) - (front.z * math::Sin(angle));
		z = (front.x * math::Sin(angle)) + (front.z * math::Cos(angle));

		totalDirection += float3(x, 0, z);
	}

	if (horizontalDirection == JoystickHorizontalDirection::NONE &&
		verticalDirection == JoystickVerticalDirection::NONE)
	{
		totalDirection = float3::zero;
	}

	// Forward
	if (input->GetKey(SDL_SCANCODE_W) != KeyState::IDLE)
	{
		totalDirection += cameraFrustum.Front().Normalized();
		currentMovements |= MovementFlag::W_DOWN;
	}

	// Back
	if (input->GetKey(SDL_SCANCODE_S) != KeyState::IDLE)
	{
		totalDirection -= cameraFrustum.Front().Normalized();
		currentMovements |= MovementFlag::S_DOWN;
	}

	// Right
	if (input->GetKey(SDL_SCANCODE_D) != KeyState::IDLE)
	{
		totalDirection += cameraFrustum.WorldRight().Normalized();
		currentMovements |= MovementFlag::D_DOWN;
	}

	// Left
	if (input->GetKey(SDL_SCANCODE_A) != KeyState::IDLE)
	{
		totalDirection -= cameraFrustum.WorldRight().Normalized();
		currentMovements |= MovementFlag::A_DOWN;
	}

	if (previousMovements ^ currentMovements)
	{
		cameraFrustum = *camera->GetFrustum();
	}

	if (playerManager->GetPlayerState() != PlayerActions::IDLE &&
		playerManager->GetPlayerState() != PlayerActions::DASHING)
	{
		if (totalDirection.IsZero())
		{
			componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
			componentAnimation->SetParameter("IsRunning", false);

			if (playerManager->GetPlayerState() == PlayerActions::WALKING)
			{
				playerManager->SetPlayerState(PlayerActions::IDLE);
			}
		}
		else 
		{
			// Low velocity while attacking
			if (playerAttackScript->IsInAttackAnimation())
			{
				newSpeed = newSpeed / lightAttacksMoveFactor;
			}

			componentAnimation->SetParameter("IsRunning", true);

			totalDirection.y = 0;
			totalDirection = totalDirection.Normalized();

			if (!IsMovementAttacking())
			{
				desiredRotation = totalDirection;
			}

			
			movement = btVector3(totalDirection.x, totalDirection.y, totalDirection.z) * deltaTime * newSpeed;
		}
	}
	else if (playerManager->GetPlayerState() == PlayerActions::IDLE && !totalDirection.IsZero())
	{
		if (playerAttackScript->IsMelee())
		{
			if (IsOnWater())
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_BIX_WATER);
			}
			else
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_BIX_METAL);
			}
		}
		else
		{
			if (IsOnWater())
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_ALLURA_WATER);
			}
			else
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_ALLURA_METAL);
			}
		}
		componentAnimation->SetParameter("IsRunning", true);
		playerManager->SetPlayerState(PlayerActions::WALKING);
	}

	if (playerManager->GetPlayerState() != PlayerActions::DASHING)
	{
		btVector3 newVelocity(movement.getX(), btRigidbody->getLinearVelocity().getY(), movement.getZ());
		btRigidbody->setLinearVelocity(newVelocity);
	}
}

void PlayerMoveScript::MoveRotate(float deltaTime)
{
	float desiredVelocity = playerManager->GetPlayerRotationSpeed();
	//Look at enemy selected while attacking
	if (IsMovementAttacking())
	{
		GameObject* enemyGO = playerAttackScript->GetEnemyDetected();
		ComponentTransform* enemy = enemyGO->GetComponent<ComponentTransform>();

		float3 vecTowardsEnemy = (enemy->GetGlobalPosition() - componentTransform->GetGlobalPosition()).Normalized();
		desiredRotation = vecTowardsEnemy;

		desiredVelocity *= rotationAttackVelocity * deltaTime;
	}

	desiredRotation.y = 0;
	desiredRotation = desiredRotation.Normalized();

	btTransform worldTransform = btRigidbody->getWorldTransform();
	Quat rot = Quat::LookAt(componentTransform->GetGlobalForward().Normalized(), desiredRotation, float3::unitY, float3::unitY);
	Quat rotation = componentTransform->GetGlobalRotation();
	Quat targetRotation = rot * componentTransform->GetGlobalRotation();

	Quat rotationError = targetRotation * rotation.Normalized().Inverted();
	rotationError.Normalize();

	if (!rotationError.Equals(Quat::identity, 0.05f))
	{
		float3 axis;
		float angle;
		rotationError.ToAxisAngle(axis, angle);
		axis.Normalize();

		float3 velocityRotation = axis * angle * desiredVelocity;
		Quat angularVelocityQuat(velocityRotation.x, velocityRotation.y, velocityRotation.z, 0.0f);
		Quat wq_0 = angularVelocityQuat * rotation;

		float deltaValue = 0.5f * deltaTime;
		Quat deltaRotation = Quat(deltaValue * wq_0.x,
			deltaValue * wq_0.y,
			deltaValue * wq_0.z,
			deltaValue * wq_0.w);

		if (deltaRotation.Length() > rotationError.Length())
		{
			worldTransform.setRotation({ targetRotation.x,
				targetRotation.y,
				targetRotation.z,
				targetRotation.w });
		}
		else
		{
			Quat nextRotation(rotation.x + deltaRotation.x,
				rotation.y + deltaRotation.y,
				rotation.z + deltaRotation.z,
				rotation.w + deltaRotation.w);
			nextRotation.Normalize();

			worldTransform.setRotation({ nextRotation.x,
				nextRotation.y,
				nextRotation.z,
				nextRotation.w });
		}
	}

	btMatrix3x3 rbmatrix = worldTransform.getBasis();

	btQuaternion quat;
	rbmatrix.extractRotation(quat);

	btRigidbody->setWorldTransform(worldTransform);
	btRigidbody->getMotionState()->setWorldTransform(worldTransform);
}

bool PlayerMoveScript::CheckRightTrigger() 
{
	if (input->GetRightTriggerIntensity() == 0)
	{
		rightTrigger = false;
		return false;
	}
	else if (!rightTrigger && input->GetRightTriggerIntensity() > 16.383)
	{
		rightTrigger = true;
		return true;
	}
	return false;
}

void PlayerMoveScript::DashRoll(float deltaTime)
{
	if(ghostBixDashing) 
	{
		for (GameObject* child : dashBix->GetChildren()) 
		{
			if (child->GetTag() == "Effect" && child->GetComponent<ComponentAnimation>()->GetActualStateName() != "DashingInit" &&
				child->GetComponent<ComponentAnimation>()->GetActualStateName() != "DashingKeep" &&
				child->GetComponent<ComponentAnimation>()->GetActualStateName() != "DashingEnd")
			{
				dashBix->Disable();
				ghostBixDashing = false;
			}
		}
	}
	if (playerAttackScript->IsAttackAvailable() &&
		timeSinceLastDash > dashRollCooldown &&
		(playerManager->GetPlayerState() == PlayerActions::IDLE ||
		playerManager->GetPlayerState() == PlayerActions::WALKING) &&
		(CheckRightTrigger() ||
		isTriggeringStoredDash))
	{
		// Start a dash
		dashRollTime = 0.0f;
		timeSinceLastDash = 0.0f;
		isTriggeringStoredDash = false;
		if(dashBix != nullptr) dashBix->Enable();
		if (rollAllura != nullptr)
		{
			rollAllura->GetComponent<ComponentParticleSystem>()->Stop();
			rollAllura->Enable();
			rollAllura->GetComponent<ComponentParticleSystem>()->Play();
		}
		componentAnimation->SetParameter("IsDashing", true);
		componentAnimation->SetParameter("IsRunning", false);
		playerManager->SetPlayerState(PlayerActions::DASHING);

		JoystickHorizontalDirection horizontalDirection = input->GetLeftJoystickDirection().horizontalDirection;
		JoystickVerticalDirection verticalDirection = input->GetLeftJoystickDirection().verticalDirection;

		if (horizontalDirection == JoystickHorizontalDirection::NONE &&
			verticalDirection == JoystickVerticalDirection::NONE)
		{
			totalDirection += componentTransform->GetGlobalForward();
		}

		totalDirection.Normalize();

		btVector3 btDashDirection = btVector3(totalDirection.x, 0.0f, totalDirection.z);
		btRigidbody->setLinearVelocity(btDashDirection * dashForce);

		componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);

		if (playerAttackScript->IsMelee())
		{
			componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::DASH);
		}
		else
		{
			componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::ROLL);
		}
	}
	else
	{
		if (CheckRightTrigger() &&
			(playerManager->GetPlayerState() == PlayerActions::IDLE ||
			playerManager->GetPlayerState() == PlayerActions::WALKING))
		{
			isTriggeringStoredDash = true;
		}
		timeSinceLastDash += deltaTime;
	}
	
	bool dashAnimationHasFinished = componentAnimation->GetActualStateName() != "DashingInit" &&
		componentAnimation->GetActualStateName() != "DashingKeep" &&
		componentAnimation->GetActualStateName() != "DashingEnd";

	if (playerManager->GetPlayerState() == PlayerActions::DASHING)
	{
		// Stop the dash
		if (dashRollTime > dashRollDuration)
		{
			if (dashAnimationHasFinished)
			{
				playerManager->SetPlayerState(PlayerActions::IDLE);
			}
			timeSinceLastDash = 0.0f;
			componentAnimation->SetParameter("IsDashing", false);
			if (dashBix != nullptr) 
			{
				ghostBixDashing = true;
			}
			if(rollAllura != nullptr) 
			{
				rollAllura->Disable();
			}
			btRigidbody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
		}
		else
		{
			dashRollTime += deltaTime;
		}
	}
}

void PlayerMoveScript::OnCollisionEnter(ComponentRigidBody* other)
{
	
	if (other->GetOwner()->CompareTag("Water"))
	{
		waterCounter++;
		if (IsOnWater() && playerManager->GetPlayerState() == PlayerActions::WALKING)
		{
			if (playerAttackScript->IsMelee())
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_BIX_WATER);
			}
			else
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_ALLURA_WATER);
			}
		}
	}
}

void PlayerMoveScript::OnCollisionExit(ComponentRigidBody* other)
{
	if (other->GetOwner()->CompareTag("Water"))
	{
		waterCounter--;
		if (!IsOnWater() && playerManager->GetPlayerState() == PlayerActions::WALKING)
		{
			if (playerAttackScript->IsMelee())
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_BIX_METAL);
			}
			else
			{
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_STOP);
				componentAudio->PostEvent(AUDIO::SFX::PLAYER::LOCOMOTION::FOOTSTEPS_WALK_ALLURA_METAL);
			}
		}
	}
}

bool PlayerMoveScript::IsParalyzed() const
{
	return isParalyzed;
}

void PlayerMoveScript::SetIsParalyzed(bool isParalyzed)
{
	this->isParalyzed = isParalyzed;
}

bool PlayerMoveScript::IsOnWater() const
{
	return waterCounter > 0;
}

PlayerJumpScript* PlayerMoveScript::GetJumpScript() const
{
	return jumpScript;
}

bool PlayerMoveScript::IsMovementAttacking() const
{
	AttackType currentAttack = playerAttackScript->GetCurrentAttackType();
	GameObject* enemyGO = playerAttackScript->GetEnemyDetected();
	if (enemyGO != nullptr && currentAttack != AttackType::NONE)
	{
		switch (currentAttack)
		{
		case AttackType::LIGHTNORMAL:
		case AttackType::HEAVYNORMAL:
		case AttackType::LIGHTFINISHER:
			return true;
		}
	}
	return false;
}
bool PlayerMoveScript::IsTriggeringStoredDash() const
{
	return isTriggeringStoredDash;
}

void PlayerMoveScript::SetIsTriggeringStoredDash(bool isTriggeringStoredDash)
{
	this->isTriggeringStoredDash = isTriggeringStoredDash;
}
