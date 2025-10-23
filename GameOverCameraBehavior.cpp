#include "../CCL.h"
#include "DrawComponents.h"
#include "../GAME/GameComponents.h"
#include "../UTIL/Utilities.h"

void GameOverCameraTag(entt::registry& registry, entt::entity entity)
{
	auto camera = registry.view<DRAW::Camera>().front();
	registry.emplace<DRAW::GameOverCamera>(camera);
}

// This function tags the current camera as the Game Over camera when the game enters the Game Over state
// In the Game Over state, the camera will lerp towards the player's position and then orbit around them
void Update_GameOverCamera(entt::registry& registry, entt::entity entity)
{
	// lerp camera close to player then stop
	float deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;
	auto& camera = registry.get<DRAW::Camera>(entity);
	auto player = registry.view<GAME::Player>().front();
	auto transform = registry.get<GAME::Transform>(player);
	if (!registry.all_of<DRAW::LerpedCamera>(entity))
	{
		if (camera.camMatrix.data[13] - transform.transform.data[13] <= 8.1f)
		{
			registry.emplace<DRAW::LerpedCamera>(entity);
		}
		// Lerp Camera Position to above player and offset down
		GW::MATH::GVECTORF targetPosition = transform.transform.row4;
		targetPosition.y += 8.0f; // offset above player
		targetPosition.z -= 5.0f; // offset behind player
		GW::MATH::GVECTORF currentPosition = camera.camMatrix.row4;
		GW::MATH::GVECTORF newPosition;
		GW::MATH::GVector::LerpF(currentPosition, targetPosition, deltaTime, newPosition);
		camera.camMatrix.row4 = newPosition;
		// Lerp Camera Rotation to look at player
		GW::MATH::GMATRIXF lookAt;
		GW::MATH::GMatrix::LookAtLHF(camera.camMatrix.row4, transform.transform.row4, GW::MATH::GVECTORF{ 0,1,0,0 }, lookAt);
		GW::MATH::GMatrix::InverseF(lookAt, lookAt);
		GW::MATH::GVECTORF row1CurrentPosition = lookAt.row1;
		GW::MATH::GVECTORF row2CurrentPosition = lookAt.row2;
		GW::MATH::GVECTORF row3CurrentPosition = lookAt.row3;
		GW::MATH::GVector::LerpF(camera.camMatrix.row1, row1CurrentPosition, deltaTime, lookAt.row1);
		GW::MATH::GVector::LerpF(camera.camMatrix.row2, row2CurrentPosition, deltaTime, lookAt.row2);
		GW::MATH::GVector::LerpF(camera.camMatrix.row3, row3CurrentPosition, deltaTime, lookAt.row3);
		camera.camMatrix.row1 = lookAt.row1;
		camera.camMatrix.row2 = lookAt.row2;
		camera.camMatrix.row3 = lookAt.row3;
	}
	if (registry.all_of<DRAW::LerpedCamera>(entity))
	{
		// Rotate Camera around player slowly
		float radius = 5.0f;
		// Start at pi to begin behind player
		static float angle = 3.14159f;
		angle += deltaTime * 0.5f; // rotate at 0.5 radians per second
		GW::MATH::GVECTORF camPos =
		{
			transform.transform.data[12] + radius * sinf(angle),
			camera.camMatrix.data[13],
			transform.transform.data[14] + radius * cosf(angle),
			1.0f
		};
		// Always look at player
		GW::MATH::GMATRIXF lookAt;
		GW::MATH::GMatrix::LookAtLHF(camPos, transform.transform.row4, GW::MATH::GVECTORF{ 0,1,0,0 }, lookAt);
		GW::MATH::GMatrix::InverseF(lookAt, lookAt);
		camera.camMatrix.row1 = lookAt.row1;
		camera.camMatrix.row2 = lookAt.row2;
		camera.camMatrix.row3 = lookAt.row3;
		camera.camMatrix.row4 = camPos;
	}
}

CONNECT_COMPONENT_LOGIC()
{
	// Game Over Camera Behavior
	registry.on_construct<GAME::GameOver>().connect<GameOverCameraTag>();
	registry.on_update<DRAW::GameOverCamera>().connect<Update_GameOverCamera>();
}