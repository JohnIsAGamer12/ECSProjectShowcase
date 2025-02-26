#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"
#include "../CCL.h"

namespace GAME
{
	void Construct_Bullet(entt::registry& registry, entt::entity entity)
	{
		// get the Game Config for the Bullet's Model
		std::shared_ptr<GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		// emplace important tags
		registry.emplace<GAME::Collidable>(entity);

		// emplace important components
		auto& bulletTransform = registry.emplace<GAME::Transform>(entity);
		auto& bulletVelocity = registry.emplace<GAME::Velocity>(entity);
		auto& bulletMeshCollection = registry.emplace<DRAW::MeshCollection>(entity);

		auto& modelManager = registry.ctx().get<DRAW::ModelManager>();

		// populate the bullet's mesh collection
		std::string bulletModelPath = (*config).at("Bullet").at("model").as<std::string>();
		for (int i = 0; i < modelManager.models[bulletModelPath].meshes.size(); i++)
		{
			auto bulletMesh = registry.create();

			auto& bulletMeshGeometryData = registry.emplace<DRAW::GeometryData>(bulletMesh);
			bulletMeshGeometryData.indexCount = registry.get<DRAW::GeometryData>(modelManager.models[bulletModelPath].meshes[i]).indexCount;
			bulletMeshGeometryData.indexStart = registry.get<DRAW::GeometryData>(modelManager.models[bulletModelPath].meshes[i]).indexStart;
			bulletMeshGeometryData.vertexStart = registry.get<DRAW::GeometryData>(modelManager.models[bulletModelPath].meshes[i]).vertexStart;

			auto& bulletMeshGPUInstance = registry.emplace<DRAW::GPUInstance>(bulletMesh);
			bulletMeshGPUInstance.matData = registry.get<DRAW::GPUInstance>(modelManager.models[bulletModelPath].meshes[i]).matData;
			bulletMeshGPUInstance.transform = bulletTransform.transformMatrix;

			bulletMeshCollection.meshes.push_back(bulletMesh);
		}
		bulletMeshCollection.collider = modelManager.models[bulletModelPath].collider;

	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<Bullet>().connect<Construct_Bullet>();
	}
}// namespace GAME