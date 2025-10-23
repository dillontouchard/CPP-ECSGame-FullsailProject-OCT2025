#include "../CCL.h"
#include "GameComponents.h"
#include "../UTIL/Utilities.h"
#include "../DRAW/DrawComponents.h"

// This function gets called every frame to update the enemy's behavior
// It moves the enemy towards the player's position if there are no obstacles in the way
// and roams randomly otherwise.
// The obstacle detection is done using raycasting.
// Note: Gateware's library is used for collision detection, raycasting and matrix math.
void Update_Enemy(entt::registry& registry, entt::entity entity)
{
	// config and deltaTime for setting speed of enemy movement
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
	float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();
	auto& deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;

	// Move enemy to the players position
	auto& enemyTransform = registry.get<GAME::Transform>(entity);
	auto& enemyVelocity = registry.get<GAME::Velocity>(entity);
	auto playerView = registry.view<GAME::Player>();
	if (!playerView.empty())
	{
		// Get the first player entity (assuming single player for now)
		auto playerEntity = *playerView.begin();
		auto& playerTransform = registry.get<GAME::Transform>(playerEntity);

		// Calculate direction vector from enemy to player
		GW::MATH::GVECTORF direction;
		GW::MATH::GVector::SubtractVectorF(playerTransform.transform.row4, enemyTransform.transform.row4, direction);

		// Calculate distance to player before normalizing
		float playerDistance = 0.0f;
		GW::MATH::GVector::MagnitudeF(direction, playerDistance);

		// Normalize
		GW::MATH::GVector::NormalizeF(direction, direction);

		// Remove ObstacleInWay tag if exists to avoid errors for slots already being taken on components
		if (registry.all_of<GAME::ObstacleInWay>(entity))
		{
			registry.remove<GAME::ObstacleInWay>(entity);
		}

		// Create a ray from enemy to player to pass into IntersectRayToOBBF method
		GW::MATH::GRAYF ray;
		ray.position = enemyTransform.transform.row4;
		ray.direction = direction;
		ray.direction.y = 0.1f; // Slightly offset ray upwards to avoid ground intersection
		ray.direction.w = 0.0f;
		GW::MATH::GVector::NormalizeF(ray.direction, ray.direction);

		// Get obstacles and test for intersection of collider OBB's with the ray
		auto obstacleView = registry.view<GAME::Obstacle>(entt::exclude<GAME::Player, GAME::Enemy>);
		for (auto& obstacleEntity : obstacleView)
		{
			//--------------------------------------------------------------------------------//
			// Prepare obstacle collider for intersection test by scaling, rotating, and translating to have the correct world position
			using namespace GW::MATH;
			// Get collider and transform of obstacle
			auto col = registry.get<DRAW::MeshCollection>(obstacleEntity).collider;
			auto& transA = registry.get<GAME::Transform>(obstacleEntity).transform;

			// Scale collider by transform scale
			GVECTORF vecA;
			GMatrix::GetScaleF(transA, vecA);
			col.extent.x *= vecA.x;
			col.extent.y *= vecA.y;
			col.extent.z *= vecA.z;

			// Translate collider to world position
			GMatrix::VectorXMatrixF(transA, col.center, col.center);

			// Rotate collider by transform rotation
			GQUATERNIONF qA;
			GQuaternion::SetByMatrixF(transA, qA);
			GQuaternion::MultiplyQuaternionF(col.rotation, qA, col.rotation);

			// Initialize contact point and interval
			float interval = 0.0f;
			GW::MATH::GVECTORF contactPoint;
			//--------------------------------------------------------------------------------//

			// Test ray against obstacle's collider
			GW::MATH::GCollision::GCollisionCheck result;
			GW::MATH::GCollision::IntersectRayToOBBF(ray, col, result, contactPoint, interval);
			if (result == GW::MATH::GCollision::GCollisionCheck::COLLISION && interval > 0 && interval <= playerDistance)
			{
				// Emplace ObstacleInWay tag
				registry.emplace<GAME::ObstacleInWay>(entity);
				break;
			}
		}

		// If obstacle in way then roam
		if (registry.all_of<GAME::ObstacleInWay>(entity))
		{
			// If not already roaming, set to roaming
			if (!registry.all_of<GAME::Roaming>(entity))
			{
				// Get enemy velocity and set to random direction and set enemy to roaming
				auto& moveDir = UTIL::GetRandomVelocityVector();
				moveDir.x *= enemySpeed * deltaTime;
				moveDir.z *= enemySpeed * deltaTime;
				moveDir.y = 0.0f;
				moveDir.w = 0.0f;
				enemyVelocity.direction = moveDir;
				registry.emplace<GAME::Roaming>(entity);
			}
			// Update Enemy position (keep roaming if exists) or (after roaming has been set)
			GW::MATH::GMatrix::TranslateLocalF(enemyTransform.transform, enemyVelocity.direction, enemyTransform.transform);
		}

		// If no obstacle, move towards player
		if (!registry.all_of<GAME::ObstacleInWay>(entity))
		{
			// If enemy was roaming, remove roaming tag
			if (registry.all_of<GAME::Roaming>(entity))
			{
				registry.remove<GAME::Roaming>(entity);
			}

			// Scale by speed and deltaTime
			direction.x *= enemySpeed * deltaTime;
			direction.y = 0.0f; // Keep enemy on the ground plane
			direction.z *= enemySpeed * deltaTime;
			direction.w = 0.0f;

			// Update velocity component
			enemyVelocity.direction = direction;

			// Update enemy position
			GW::MATH::GMatrix::TranslateLocalF(enemyTransform.transform, direction, enemyTransform.transform);
		}
	}
}

CONNECT_COMPONENT_LOGIC()
{
	// Register the Player component's update logic
	registry.on_update<GAME::Enemy>().connect<Update_Enemy>();
}

// In a main update loop the Update_Enemy function would be called when registry.patch<GAME::Enemy>(enemyEntity)
// was invoked on entities with the GAME::Enemy component.
// Note: This is a simplified example and does not include full pathfinding or obstacle avoidance logic.