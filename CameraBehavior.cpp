#include "../CCL.h"
#include "DrawComponents.h"
#include "../GAME/GameComponents.h"

// This function gets called every frame to update the camera's position by grabbing the player's entity and getting its transform
// then setting the camera's position to be above the player by changing the x and z matrix data of the camera to match the players.
// This is because the game used x and z for movement directions.
void Update_Camera(entt::registry& registry, entt::entity entity)
{
	// Update camera to above the player's position
	auto& camera = registry.get<DRAW::Camera>(entity);
	auto playerView = registry.view<GAME::Player>();
	if (playerView.front() != entt::null)
	{
		GAME::Transform& playerTransform = registry.get<GAME::Transform>(playerView.front());
		camera.camMatrix.data[12] = playerTransform.transform.data[12]; // Follow player's X
		camera.camMatrix.data[14] = playerTransform.transform.data[14]; // Follow player's Z
	}
}

CONNECT_COMPONENT_LOGIC()
{
	// Register the Camera component's update logic
	registry.on_update<DRAW::Camera>().connect<Update_Camera>();
}

//In a main update loop the Update_Camera function would be called when registry.patch<DRAW::Camera>(cameraEntity)
//was invoked on entities with the DRAW::Camera component.