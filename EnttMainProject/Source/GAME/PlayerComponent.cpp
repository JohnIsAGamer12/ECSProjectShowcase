#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"
#include "../CCL.h"

namespace GAME
{
	void Construct_PlayerComponent(entt::registry& registry, entt::entity entity)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		auto& playerMeshCollection = registry.emplace<DRAW::MeshCollection>(entity);
		auto& playerTransform = registry.emplace<GAME::Transform>(entity);
		auto& playerHealth = registry.emplace<GAME::Health>(entity);
		playerHealth.health = (*config).at("Player").at("hitpoints").as<float>();
		registry.emplace<GAME::Collidable>(entity);

		auto& modelManager = registry.ctx().get<DRAW::ModelManager>();

		// Load the player model
		std::string& playerModelPath = (*config).at("Player").at("model").as<std::string>();

		for (size_t i = 0; i < modelManager.models[playerModelPath].meshes.size(); i++)
		{
			auto playerMesh = registry.create();

			auto& playerMeshGeometryData = registry.emplace<DRAW::GeometryData>(playerMesh);
			playerMeshGeometryData.indexCount = registry.get<DRAW::GeometryData>(modelManager.models[playerModelPath].meshes[i]).indexCount;
			playerMeshGeometryData.indexStart = registry.get<DRAW::GeometryData>(modelManager.models[playerModelPath].meshes[i]).indexStart;
			playerMeshGeometryData.vertexStart = registry.get<DRAW::GeometryData>(modelManager.models[playerModelPath].meshes[i]).vertexStart;

			auto& playerMeshGPUInstance = registry.emplace<DRAW::GPUInstance>(playerMesh);
			playerMeshGPUInstance.matData = registry.get<DRAW::GPUInstance>(modelManager.models[playerModelPath].meshes[i]).matData;
			playerMeshGPUInstance.transform = registry.get<DRAW::GPUInstance>(modelManager.models[playerModelPath].meshes[i]).transform;

			playerMeshCollection.meshes.push_back(playerMesh);
		}
		playerTransform.transformMatrix = registry.get<DRAW::GPUInstance>(playerMeshCollection.meshes[0]).transform;
		playerMeshCollection.collider = modelManager.models[playerModelPath].collider;
	}

	void Update_PlayerComponent(entt::registry& registry, entt::entity entity)
	{
		// Get the Game Config for the Player's Max Speed
		std::shared_ptr<GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		// Get the Player speed from the .ini
		float playerSpeed = (*config).at("Player").at("speed").as<float>();
		// Get delta time
		double deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;
		// get the Player Input Manager
		auto playerInput = registry.ctx().get<UTIL::Input>();
		// get the current player's transform in the world
		auto& playerTransform = registry.get<Transform>(entity);

		// adjust player's position according to the player's input on WASD
		float wInput = 0.f, aInput = 0.f, sInput = 0.f, dInput = 0.f;
		playerInput.immediateInput.GetState(G_KEY_W, wInput);
		playerInput.immediateInput.GetState(G_KEY_A, aInput);
		playerInput.immediateInput.GetState(G_KEY_S, sInput);
		playerInput.immediateInput.GetState(G_KEY_D, dInput);

		float xPos = (dInput - aInput);
		float zPos = (wInput - sInput);

		// calculate the player's new position
		GW::MATH::GMATRIXF CopyTransform{ playerTransform.transformMatrix };
		GW::MATH::GVECTORF playerDir = GW::MATH::GVECTORF{ xPos, 0.f, zPos };

		// Normalize Speed to be consistent on all directions including diagonally
		GW::MATH::GVector::NormalizeF(playerDir, playerDir);
		GW::MATH::GVector::ScaleF(playerDir, (playerSpeed * (float)deltaTime), playerDir);

		GW::MATH::GMatrix::TranslateLocalF(CopyTransform, playerDir, CopyTransform);

		// Paste New Transform
		playerTransform.transformMatrix = CopyTransform;

		// check if the Firing Tag is present on the player entity
		auto* fireTag = registry.try_get<GAME::Firing>(entity);

		if (!fireTag)
		{
			// Firing bullets/ Instantiate Bullets
			float upInput = 0, downInput = 0, leftInput = 0, rightInput = 0;
			playerInput.immediateInput.GetState(G_KEY_UP, upInput);
			playerInput.immediateInput.GetState(G_KEY_DOWN, downInput);
			playerInput.immediateInput.GetState(G_KEY_LEFT, leftInput);
			playerInput.immediateInput.GetState(G_KEY_RIGHT, rightInput);

			if (upInput > 0 || downInput > 0 || rightInput > 0 || leftInput > 0)
			{
				// put the firing state on the player
				auto& firingTag = registry.emplace<GAME::Firing>(entity);
				firingTag.cooldown = (*config).at("Player").at("firerate").as<float>();

				// Create the Bullet Entity
				auto bullet = registry.create();
				registry.emplace<Bullet>(bullet);
				
				// Fill in the Bullet's Transform Component
				if (auto* bulletTransform = registry.try_get<GAME::Transform>(bullet))
					bulletTransform->transformMatrix = playerTransform.transformMatrix;

				// fill in the Bullet's Velocity Component
				if (auto* velocity = registry.try_get<GAME::Velocity>(bullet))
				{
					// Get Bullet Speed
					float bulletSpeed = (*config).at("Bullet").at("speed").as<float>();

					float zPos = upInput - downInput;
					float xPos = rightInput - leftInput;

					GW::MATH::GVECTORF intialBulletVelocity{ xPos, 0.f, zPos };
					GW::MATH::GVector::NormalizeF(intialBulletVelocity, intialBulletVelocity);
					GW::MATH::GVector::ScaleF(intialBulletVelocity, bulletSpeed, velocity->_velocity);
				}
			}
		}
		else
		{
			// decrement the cooldown
			fireTag->cooldown -= (float)deltaTime;
			if (fireTag->cooldown <= 0)
			{
				// remove the firing tag
				registry.remove<GAME::Firing>(entity);
			}
		}


		// check if the Invulernability Tag is present on the player entity
		auto* invulernabilityTag = registry.try_get<GAME::Invulernability>(entity);

		if (invulernabilityTag)
		{
			// decrement the cooldown
			invulernabilityTag->invulernabilityTime -= (float)deltaTime;
			if (invulernabilityTag->invulernabilityTime <= 0)
			{
				// remove the Invernability Component and Damaged tag
				registry.remove<GAME::Invulernability>(entity);
			}
		}
	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<Player>().connect<Construct_PlayerComponent>();
		registry.on_update<Player>().connect<Update_PlayerComponent>();
	}

}// namespace GAME