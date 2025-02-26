#include "GameComponents.h"
#include "../CCL.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME
{

#pragma region Helper Function Declaration

	void PlayerTakeDamage(entt::registry& registry, entt::entity targetEntity, std::shared_ptr<const GameConfig>& config);
	void BounceTheEnemy(GW::MATH::GOBBF& colliderBox, GW::MATH::GVECTORF& enemyPosition, GW::MATH::GVECTORF& enemyVelocity);

#pragma endregion
	
	void Update_GameManager(entt::registry& registry, entt::entity entity)
	{
		if (registry.all_of<GameOver>(entity))
			return;

		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		// get the delta time
		auto& deltaTime = registry.ctx().get<UTIL::DeltaTime>().dtSec;

		// update all Entites with the Velocity Component
		auto velocityView = registry.view<Velocity, Transform>();

		velocityView.each([&](Velocity& velocity, Transform& transform)
			{
				Transform copyTransform;
				copyTransform.transformMatrix = transform.transformMatrix;

				// update the position of the entity based on the velocity
				GW::MATH::GVECTORF velocityVector = velocity._velocity;

				velocityVector.x *= deltaTime;
				velocityVector.y *= deltaTime;
				velocityVector.z *= deltaTime;

				GW::MATH::GMatrix::TranslateGlobalF(copyTransform.transformMatrix, velocityVector, transform.transformMatrix);
			});

		// view all entities with a Transform & MeshCollection component
		auto view = registry.view<Transform, DRAW::MeshCollection>();

		view.each([&](Transform _transform, DRAW::MeshCollection _meshCollection)
			{
				// copy the matrix that is in the Transform to each mesh GPUInstance's transform
				Transform copyTransform;
				copyTransform.transformMatrix = _transform.transformMatrix;

				for (int i = 0; i < _meshCollection.meshes.size(); i++)
				{
					auto& mesh = _meshCollection.meshes[i];
					auto& gpuInstance = registry.get<DRAW::GPUInstance>(mesh);
					gpuInstance.transform = copyTransform.transformMatrix;
				}
			});

		//updating all Player Entities
		auto playerView = registry.view<Player>();

		int amountOfDeadPlayers = 0;
		for (auto playerEntity : playerView)
		{
			// check if each player's health is <= 0
			auto* health = registry.try_get<Health>(playerEntity);

			if (health->health <= 0)
				amountOfDeadPlayers++;

			registry.patch<GAME::Player>(playerEntity);
		}

		if (amountOfDeadPlayers >= playerView.size())
		{
			registry.emplace<GameOver>(entity);
			std::cout << "You Died, Game Over!" << std::endl;
		}
		auto& enemyView = registry.view<Enemy>();

		if (enemyView.size() <= 0)
		{
			registry.emplace<GameOver>(entity);
			std::cout << "You Win, Good Job!" << std::endl;
		}

		// check for collisions
		auto collidableView = registry.view<Collidable, DRAW::MeshCollection, Transform>();

		for (auto enttA = collidableView.begin(); enttA != collidableView.end(); enttA++)
		{
			auto colliderA = registry.get<DRAW::MeshCollection>(*enttA).collider;
			auto& transformA = registry.get<Transform>(*enttA).transformMatrix;

			GW::MATH::GVECTORF vecA;
			GW::MATH::GMatrix::GetScaleF(transformA, vecA);
			colliderA.extent.x *= vecA.x;
			colliderA.extent.y *= vecA.y;
			colliderA.extent.z *= vecA.z;

			// convert transformA into world space
			GW::MATH::GMatrix::VectorXMatrixF(transformA, colliderA.center, colliderA.center);

			// Rotate the Collider
			GW::MATH::GQUATERNIONF quatA;
			GW::MATH::GQuaternion::SetByMatrixF(transformA, quatA);
			GW::MATH::GQuaternion::MultiplyQuaternionF(colliderA.rotation, quatA, colliderA.rotation);

			auto enttB = enttA;
			for (enttB++; enttB != collidableView.end(); enttB++)
			{
				auto colliderB = registry.get<DRAW::MeshCollection>(*enttB).collider;
				auto& transformB = registry.get<Transform>(*enttB).transformMatrix;

				GW::MATH::GVECTORF vecB;
				GW::MATH::GMatrix::GetScaleF(transformB, vecB);
				colliderB.extent.x *= vecB.x;
				colliderB.extent.y *= vecB.y;
				colliderB.extent.z *= vecB.z;

				// convert transformA into world space
				GW::MATH::GMatrix::VectorXMatrixF(transformB, colliderB.center, colliderB.center);

				// Rotate the Collider
				GW::MATH::GQUATERNIONF quatB;
				GW::MATH::GQuaternion::SetByMatrixF(transformB, quatB);
				GW::MATH::GQuaternion::MultiplyQuaternionF(colliderB.rotation, quatB, colliderB.rotation);

				GW::MATH::GCollision::GCollisionCheck result;
				GW::MATH::GCollision::TestOBBToOBBF(colliderA, colliderB, result);

				if (GW::MATH::GCollision::GCollisionCheck::COLLISION == result)
				{
					// Collision detected					
					// Bullet to Wall
					if (registry.all_of<Bullet>(*enttA) && registry.all_of<Obstacle>(*enttB))
					{
						registry.emplace_or_replace<ToDestroy>(*enttA);
					}
					if (registry.all_of<Bullet>(*enttB) && registry.all_of<Obstacle>(*enttA))
					{

						registry.emplace_or_replace<ToDestroy>(*enttB);
					}

					// Bullet to Enemy
					if (registry.all_of<Bullet>(*enttA) && registry.all_of<Enemy>(*enttB))
					{
						registry.emplace_or_replace<ToDestroy>(*enttA);

						// have Enemy take damage
						auto& health = registry.get<Health>(*enttB).health;
						health -= 1.f;
					}
					if (registry.all_of<Bullet>(*enttB) && registry.all_of<Enemy>(*enttA))
					{
						registry.emplace_or_replace<ToDestroy>(*enttB);

						// have enemy take damage
						auto& health = registry.get<Health>(*enttA).health;
						health -= 1.f;
					}

					// Enemy to Wall
					if (registry.all_of<Enemy>(*enttA) && registry.all_of<Obstacle>(*enttB))
					{
						// get the enemies current velcity
						auto& velocity = registry.get<Velocity>(*enttA)._velocity;

						BounceTheEnemy(colliderB, transformA.row4, velocity);
					}
					if (registry.all_of<Enemy>(*enttB) && registry.all_of<Obstacle>(*enttA))
					{
						// get the enemies current velcity
						auto& velocity = registry.get<Velocity>(*enttA)._velocity;

						BounceTheEnemy(colliderA, transformB.row4, velocity);
					}


					// Enemy to Player
					if (registry.all_of<Enemy>(*enttA) && registry.all_of<Player>(*enttB))
					{
						PlayerTakeDamage(registry, *enttB, config);
					}
					if (registry.all_of<Enemy>(*enttB) && registry.all_of<Player>(*enttA))
					{
						PlayerTakeDamage(registry, *enttA, config);
					}
				}
			}
		}

		// systems to shatter enemies
		auto shatterView = registry.view<Health, Enemy>();

		// check each eney if it's 0 or less and handle the destruction of the enemy
		for (auto entt : shatterView)
		{
			auto& currEnemyhealth = registry.get<Health>(entt).health;
			auto* currEnemyShatter = registry.try_get<Shatters>(entt);

			if (currEnemyhealth <= 0)
			{
				registry.emplace_or_replace<ToDestroy>(entt);

				if (currEnemyShatter == nullptr)
					continue;

				// create the shatter effect
				if (currEnemyShatter->shatterCount > 0)
				{
					for (int i = 0; i < currEnemyShatter->shatterAmount; i++)
					{
						auto enemyDupe = registry.create();
						registry.emplace<Enemy>(enemyDupe);

						auto& enemyDupeTransform = registry.get<Transform>(enemyDupe);

						enemyDupeTransform.transformMatrix = registry.get<Transform>(entt).transformMatrix;

						auto& enemyDupeShatter = registry.get<Shatters>(enemyDupe);
						enemyDupeShatter.shatterCount = currEnemyShatter->shatterCount - 1;

						if (enemyDupeShatter.shatterCount <= 0)
						{
							// remove the shatter component
							registry.remove<Shatters>(enemyDupe);
						}

						float shatterScale = (*config).at("Enemy1").at("shatterScale").as<float>();

						GW::MATH::GMatrix::ScaleLocalF(enemyDupeTransform.transformMatrix, GW::MATH::GVECTORF{ shatterScale, shatterScale, shatterScale }, enemyDupeTransform.transformMatrix);
					}
				}
			}
		}

		auto toDestroyView = registry.view<ToDestroy>();

		for (auto entt : toDestroyView)
		{
			registry.destroy(entt);
		}
	}

#pragma region Helper Function Implementation

	void BounceTheEnemy(GW::MATH::GOBBF& colliderBox, GW::MATH::GVECTORF& enemyPosition, GW::MATH::GVECTORF& enemyVelocity)
	{
		GW::MATH::GVECTORF point;
		GW::MATH::GCollision::ClosestPointToOBBF(colliderBox, enemyPosition, point);

		GW::MATH::GVECTORF normal;
		GW::MATH::GVector::SubtractVectorF(enemyPosition, point, normal);
		normal.y = 0.f; normal.w = 0.f;
		GW::MATH::GVector::NormalizeF(normal, normal);


		// w = v - (2 * (v * n) * n);
		float dot = 0.f;
		GW::MATH::GVector::DotF(enemyVelocity, normal, dot);
		dot *= 2.f;
		GW::MATH::GVector::ScaleF(normal, dot, normal);
		GW::MATH::GVector::SubtractVectorF(enemyVelocity, normal, enemyVelocity);
	}

	void PlayerTakeDamage(entt::registry& registry, entt::entity targetEntity, std::shared_ptr<const GameConfig>& config)
	{
		if (!registry.all_of<Invulernability>(targetEntity))
		{
			// deal damage to targetEntity
			auto* health = registry.try_get<Health>(targetEntity);

			if (health)
			{
				health->health -= 1.f;
				std::cout << "Hit! Player Has: " << health->health << " Health Points Left!" << std::endl;

				// add the invulernability tag
				auto& invulernability = registry.emplace<Invulernability>(targetEntity);
				invulernability.invulernabilityTime = (*config).at("Player").at("invulnPeriod").as<float>();
			}
		}
	}

#pragma endregion

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_update<GameManager>().connect<Update_GameManager>();
	}

}// namespace GAME