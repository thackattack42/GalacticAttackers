// The Enemy system is responsible for enemy behaviors
#ifndef ENEMYLOGIC_H
#define ENEMYLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"
#include "../Entities/EnemyData.h"
#include "../Components/Physics.h"


// example space game (avoid name collisions)
namespace GA
{
	class EnemyLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// handle to events
		GW::CORE::GEventGenerator eventPusher;
		std::shared_ptr<Level_Data> levelData;
		bool shieldon1 = true;
		bool shieldon2 = true;
		bool shieldon3 = true;

	public:
		// attach the required logic to the ECS 
		bool Init(std::shared_ptr<flecs::world> _game,
			std::weak_ptr<const GameConfig> _gameConfig,
			GW::CORE::GEventGenerator _eventPusher, std::shared_ptr<Level_Data> _levelData);
		// control if the system is actively running
		bool Activate(bool runSystem);
		// release any resources allocated by the system
		bool Shutdown();
		bool moveRight;
		int timesMoved = 0;
		float timer;

	private:
		bool FireLasersEnemy(flecs::world& stage, GA::Position origin);

	};

};

#endif