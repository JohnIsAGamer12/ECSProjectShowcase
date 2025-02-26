#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	///*** Tags ***///
	struct Player { };

	struct Enemy {};

	struct Bullet {};

	struct Obstacle {};

	struct Collidable {};

	struct GameOver {};

	struct ToDestroy {};

	///*** Components ***///
	struct Health {
		float health;
	};

	struct Transform {
		GW::MATH::GMATRIXF transformMatrix;
	};

	struct Velocity {
		GW::MATH::GVECTORF _velocity;
	};

	struct Firing { 
		float cooldown = 0.f; 
		float defaultCoolDown = cooldown; 
	};

	struct Invulernability {
		float invulernabilityTime = 0.f;
		float defaultInvulernabilityTime = invulernabilityTime;
	};;

	struct Shatters {
		int shatterCount = 0, shatterAmount = 0;
	};

	struct GameManager { };

}// namespace GAME
#endif // !GAME_COMPONENTS_H_