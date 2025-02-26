#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"
#include "../CCL.h"

namespace GAME
{
	void Construct_Enemy(entt::registry& registry, entt::entity entity)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		auto& enemyHealth = registry.emplace<GAME::Health>(entity);
		enemyHealth.health = (*config).at("Enemy1").at("hitpoints").as<float>();
		auto& enemyShatters = registry.emplace<GAME::Shatters>(entity);
		enemyShatters.shatterCount = (*config).at("Enemy1").at("initialShatterCount").as<int>();
		enemyShatters.shatterAmount = (*config).at("Enemy1").at("shatterAmount").as<int>();
		auto& enemyMeshCollection = registry.emplace<DRAW::MeshCollection>(entity);
		auto& enemyTransform = registry.emplace<GAME::Transform>(entity);
		auto& enemyVelocity = registry.emplace<GAME::Velocity>(entity);
		registry.emplace<GAME::Collidable>(entity);

		// get enemy max speed
		float enemySpeed = (*config).at("Enemy1").at("speed").as<float>();

		// randomize the velocity
		enemyVelocity._velocity = UTIL::GetRandomVelocityVector();

		GW::MATH::GVector::ScaleF(enemyVelocity._velocity, enemySpeed, enemyVelocity._velocity);
	
		auto& modelManager = registry.ctx().get<DRAW::ModelManager>();

		std::string& enemyModelPath = (*config).at("Enemy1").at("model").as<std::string>();

		for (size_t i = 0; i < modelManager.models[enemyModelPath].meshes.size(); i++)
		{
			auto& enemyGeometryData = registry.emplace<DRAW::GeometryData>(entity);
			enemyGeometryData.indexCount = registry.get<DRAW::GeometryData>(modelManager.models[enemyModelPath].meshes[i]).indexCount;
			enemyGeometryData.indexStart = registry.get<DRAW::GeometryData>(modelManager.models[enemyModelPath].meshes[i]).indexStart;
			enemyGeometryData.vertexStart = registry.get<DRAW::GeometryData>(modelManager.models[enemyModelPath].meshes[i]).vertexStart;

			auto& enemyGPUInstance = registry.emplace<DRAW::GPUInstance>(entity);
			enemyGPUInstance.matData = registry.get<DRAW::GPUInstance>(modelManager.models[enemyModelPath].meshes[i]).matData;
			enemyGPUInstance.transform = registry.get<DRAW::GPUInstance>(modelManager.models[enemyModelPath].meshes[i]).transform;

			enemyMeshCollection.meshes.push_back(entity);
		}
		enemyTransform.transformMatrix = registry.get<DRAW::GPUInstance>(enemyMeshCollection.meshes[0]).transform;
		enemyMeshCollection.collider = modelManager.models[enemyModelPath].collider;
	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<Enemy>().connect<Construct_Enemy>();
	}
} // namespace GAME