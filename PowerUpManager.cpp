#include "../UTIL/Utilities.h"
#include "GameComponents.h"
#include "../CCL.h"
#include "GameplayBehavior.h"

// This file contains the logic for managing power-ups in the game,
// including spawning power-ups upon enemy death and applying their effects to the player.
// It uses the entt library for entity-component-system (ECS) architecture.
// Used in tandem with collision detection to handle power-up pickups.
// Config file is used to define power-up properties and are loaded dynamically using data oriented principles.
namespace GAME
{
	// Returns a random power-up type
	PowerUp GetRandomPowerUp()
	{
		PowerUp powerUp;
		// Converts the last enum value (Count) to an integer for the size of the enum
		int numOfPowerUps = static_cast<int>(PowerUp::Type::Count);
		powerUp.type = static_cast<PowerUp::Type>(UTIL::RandomInt(0, numOfPowerUps - 1));
		return powerUp;
	}

	// Converts PowerUp::Type enum to string for config lookup
	std::string PowerUpTypeToString(PowerUp::Type type) {
		switch (type) {
		case PowerUp::Type::SplitShot: return "SplitShot";
		case PowerUp::Type::DamageOverTime: return "DamageOverTime";
		case PowerUp::Type::ExtraLife: return "ExtraLife";
		case PowerUp::Type::DoubleScore: return "DoubleScore";
		default: return "Unknown";
		}
	}

	void AddLife(entt::registry& registry, entt::entity playerEntity, PowerUp& powerUp)
	{
		auto& health = registry.get<Health>(playerEntity);
		health.value += static_cast<int>(powerUp.modifier);
		registry.emplace<ToRemovePowerUp>(playerEntity);
	}

	// Handles spawning power-ups at the last known enemy location upon enemy death
	void HandlePowerUpSpawn(entt::registry& registry)
	{
		// Check if there are any power-up spawn requests and if random chance allows spawning
		auto& vPowerUpSpawnQueue = registry.ctx().get<PowerUpSpawnQueue>().requests;
		float randomChance = UTIL::RandomFloat(0.0f, 100.0f);
		if (vPowerUpSpawnQueue.empty() || randomChance > 25.0f)
		{
			vPowerUpSpawnQueue.clear(); // clear queue to avoid multiple spawns
			return; // 25% chance to spawn a power-up
		}

		// Config for model lookup
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		// Get the power up type
		PowerUp powerUp = GetRandomPowerUp();
		// Get the model of the power up from config
		std::string powerUpName = PowerUpTypeToString(powerUp.type);
		std::string powerUpModel = (*config).at(powerUpName).at("model").as<std::string>();

		auto newEntity = registry.create();
		registry.emplace<GAME::Transform>(newEntity);
		registry.emplace<DRAW::MeshCollection>(newEntity);
		registry.emplace<GAME::Collidable>(newEntity);
		CreateEntityFromModel(registry, powerUpModel, newEntity);
		// use the last known enemy location to spawn the power-up at
		Transform& transform = registry.get_or_emplace<Transform>(newEntity);
		transform.transform = vPowerUpSpawnQueue.front();

		switch (powerUp.type)
		{
		case PowerUp::Type::SplitShot:
			powerUp.modifier = (*config).at(powerUpName).at("modifier").as<float>();
			powerUp.duration = (*config).at(powerUpName).at("duration").as<float>();
			powerUp.angle = (*config).at(powerUpName).at("angle").as<float>();
			registry.emplace<PowerUp>(newEntity, powerUp);
			break;
		case PowerUp::Type::DamageOverTime:
			powerUp.modifier = (*config).at(powerUpName).at("modifier").as<float>();
			powerUp.duration = (*config).at(powerUpName).at("duration").as<float>();
			powerUp.damageRate = (*config).at(powerUpName).at("damageRate").as<float>();
			registry.emplace<PowerUp>(newEntity, powerUp);
			break;
		case PowerUp::Type::ExtraLife:
			powerUp.modifier = (*config).at(powerUpName).at("modifier").as<float>();
			registry.emplace<PowerUp>(newEntity, powerUp);
			break;
		case PowerUp::Type::DoubleScore:
			powerUp.modifier = (*config).at(powerUpName).at("modifier").as<float>();
			powerUp.duration = (*config).at(powerUpName).at("duration").as<float>();
			registry.emplace<PowerUp>(newEntity, powerUp);
			break;
		default:
			break;
		}

		// clear queue to avoid multiple spawns
		vPowerUpSpawnQueue.clear();
	}

	// Handles applying power-up effects to the player upon pickup
	void HandlePowerUpPickup(entt::registry& registry, entt::entity entity)
	{
		auto& playerView = registry.view<Player>();
		for (auto& player : playerView)
		{
			auto& powerUp = registry.get<PowerUp>(player);
			switch (powerUp.type)
			{
			case PowerUp::Type::DamageOverTime:
				break;
			case PowerUp::Type::ExtraLife:
				AddLife(registry, player, powerUp);
				break;
			case PowerUp::Type::DoubleScore:
				//Implemented in EnemyDeathBehavior.cpp 
				break;
			default:
				break;
			}
		}
	}

	void UpdatePlayerPowerUp(entt::registry& registry, entt::entity entity)
	{
		if (registry.all_of<ToRemovePowerUp>(entity))
		{
			registry.remove<PowerUp>(entity);
			registry.remove<HasPowerUp>(entity);
			registry.remove<ToRemovePowerUp>(entity);
			return;
		}
		if (!registry.all_of<PowerUp>(entity))
		{
			return;
		}
		auto& powerUp = registry.get<PowerUp>(entity);
		auto& deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;
		// Decrease duration
		powerUp.duration -= static_cast<float>(deltaTime);
		if (powerUp.duration <= 0.0f)
		{
			registry.emplace<ToRemovePowerUp>(entity);
		}
	}


	CONNECT_COMPONENT_LOGIC()
	{
		// Connects the power-up spawn upon enemy death
		registry.on_destroy<Enemy>().connect<HandlePowerUpSpawn>();
		// Connects the power-up pickup logic when player collides with a power-up
		registry.on_construct<HasPowerUp>().connect<HandlePowerUpPickup>();
		// Connects the power-up update logic to handle duration and removal
		registry.on_update<Player>().connect<UpdatePlayerPowerUp>();
	}
}