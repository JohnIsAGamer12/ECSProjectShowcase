#include "DrawComponents.h"
#include "../GAME/GameComponents.h"
#include "../CCL.h"

namespace DRAW
{
	//*** HELPERS ***//


	//*** SYSTEMS ***//
	void Construct_CPULevel(entt::registry& registry, entt::entity entity)
	{
		// load Level Data
		auto& cpuLevel = registry.get<CPULevel>(entity);
		GW::SYSTEM::GLog log;
		log.Create("LevelLoading");

		bool logToConsole = true;
		log.EnableConsoleLogging(logToConsole);

		bool ret = cpuLevel.levelData.LoadLevel(cpuLevel.levelPath.c_str(), cpuLevel.modelPath.c_str(), log);
		if (!ret)
		{
			log.LogCategorized("ERROR", "Failed to load Level Data");
		}

	}

	void Update_CPULevel(entt::registry& registry, entt::entity entity)
	{
		// auto& cpuLevel = registry.get<CPULevel>(entity);
	}

	// Forward Declaration
	void Destroy_CPULevel(entt::registry& registry, entt::entity entity);

	void Construct_GPULevel(entt::registry& registry, entt::entity entity)
	{
		auto* cpuLevel = registry.try_get<CPULevel>(entity);
		GW::SYSTEM::GLog log;
		log.Create("LevelLoading");

		bool logToConsole = true;
		log.EnableConsoleLogging(logToConsole);

		if (cpuLevel)
		{
			auto& vertexBuffer = registry.emplace<VulkanVertexBuffer>(entity);
			auto& indexBuffer = registry.emplace<VulkanIndexBuffer>(entity);

			auto& vertex_Data = registry.emplace<std::vector<H2B::VERTEX>>(entity);
			auto& index_Data = registry.emplace<std::vector<unsigned int>>(entity);


			// upload the vertex buffer to the GPU
			registry.patch<VulkanVertexBuffer>(entity, [&](auto& vertex_buffer)
				{
					vertex_Data = cpuLevel->levelData.levelVertices;
				});
			// upload the index buffer to the GPU
			registry.patch<VulkanIndexBuffer>(entity, [&](auto& index_buffer)
				{
					index_Data = cpuLevel->levelData.levelIndices;
				});

			// Manages all models in the level
			auto& modelManager = registry.ctx().emplace<ModelManager>();

			// populate the model Manager from the level data
			for (auto& model : cpuLevel->levelData.blenderObjects)
			{
				MeshCollection meshCollection;
				meshCollection.collider = cpuLevel->levelData.levelColliders[cpuLevel->levelData.levelModels[model.modelIndex].colliderIndex];

				for (int i = 0; i < cpuLevel->levelData.levelModels[model.modelIndex].meshCount; i++)
				{
					auto meshEntity = registry.create();

					// fill in GeometryData
					auto& meshGeometry = registry.emplace<GeometryData>(meshEntity);
					meshGeometry.vertexStart = cpuLevel->levelData.levelModels[model.modelIndex].vertexStart;
					meshGeometry.indexStart = cpuLevel->levelData.levelModels[model.modelIndex].indexStart + cpuLevel->levelData.levelMeshes[cpuLevel->levelData.levelModels[model.modelIndex].meshStart + i].drawInfo.indexOffset;
					meshGeometry.indexCount = cpuLevel->levelData.levelMeshes[cpuLevel->levelData.levelModels[model.modelIndex].meshStart + i].drawInfo.indexCount;

					auto& meshGPUInstance = registry.emplace<GPUInstance>(meshEntity);
					meshGPUInstance.transform = cpuLevel->levelData.levelTransforms[model.transformIndex];
					meshGPUInstance.matData = cpuLevel->levelData.levelMaterials[cpuLevel->levelData.levelModels[model.modelIndex].materialStart + cpuLevel->levelData.levelMeshes[cpuLevel->levelData.levelModels[model.modelIndex].meshStart + i].materialIndex].attrib;


					// check if the current model is dynamic
					if (cpuLevel->levelData.levelModels[model.modelIndex].isDynamic)
					{
						registry.emplace<DoNotRender>(meshEntity);
						meshCollection.meshes.push_back(meshEntity);
					}
				}

				if (cpuLevel->levelData.levelModels[model.modelIndex].isDynamic)
				{
					modelManager.models[model.blendername] = meshCollection;
				}

				// check if the current model is collidable
				if (cpuLevel->levelData.levelModels[model.modelIndex].isCollidable)
				{
					auto wallCollider = registry.create();

					auto& nameOfWall = registry.emplace<std::string>(wallCollider);
					nameOfWall = model.blendername;

					auto& wallColliderTransform = registry.emplace<GAME::Transform>(wallCollider);
					wallColliderTransform.transformMatrix = cpuLevel->levelData.levelTransforms[model.transformIndex];
					auto& wallColliderMeshCollection = registry.emplace<MeshCollection>(wallCollider);
					wallColliderMeshCollection.collider = meshCollection.collider;

					registry.emplace<GAME::Collidable>(wallCollider);
					registry.emplace<GAME::Obstacle>(wallCollider);
				}
			}
		}
		else
		{
			log.LogCategorized("ERROR", "CPULevel is NULL");
		}
	}

	void Update_GPULevel(entt::registry& registry, entt::entity entity)
	{
	}

	// Forward Declaration
	void Destroy_GPULevel(entt::registry& registry, entt::entity entity);

	void Destroy_MeshCollection(entt::registry& registry, entt::entity entity);

	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<CPULevel>().connect<Construct_CPULevel>();
		registry.on_update<CPULevel>().connect<Update_CPULevel>();
		registry.on_destroy<CPULevel>().connect<Destroy_CPULevel>();

		registry.on_construct<GPULevel>().connect<Construct_GPULevel>();
		registry.on_update<GPULevel>().connect<Update_GPULevel>();
		registry.on_destroy<GPULevel>().connect<Destroy_GPULevel>();

		registry.on_destroy<MeshCollection>().connect<Destroy_MeshCollection>();
	}

	void Destroy_MeshCollection(entt::registry& registry, entt::entity entity)
	{
		auto* meshCollection = registry.try_get<DRAW::MeshCollection>(entity);

		while (meshCollection->meshes.size() > 0)
		{
			// prevents any call stack issues of destroying the same entity that we are in the process of destroying
			if (entity == meshCollection->meshes[0])
			{
				meshCollection->meshes.erase(meshCollection->meshes.begin());
				continue;
			}

			registry.destroy(meshCollection->meshes[0]);
			meshCollection->meshes.erase(meshCollection->meshes.begin());
		}
		meshCollection->meshes.clear();
	}

	void Destroy_CPULevel(entt::registry& registry, entt::entity entity)
	{
		// registry.remove<CPULevel>(entity);
	}

	void Destroy_GPULevel(entt::registry& registry, entt::entity entity)
	{
		// registry.remove<GPULevel>(entity);
	}

#pragma region Helper Function



#pragma endregion

} // namespace DRAW