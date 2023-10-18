#include <random>
#include "EnemyLogic.h"
#include "../Components/Physics.h"
#include "../Components/Identification.h"
#include "../Components/Gameplay.h"
#include "../Components/Visuals.h"
#include "../Events/Playevents.h"
#include "../Components/Components.h"
#include "../Entities/Prefabs.h"

using namespace ESG; // Example Space Game

// Connects logic to traverse any players and allow a controller to manipulate them
bool ESG::EnemyLogic::Init(std::shared_ptr<flecs::world> _game,
	std::weak_ptr<const GameConfig> _gameConfig,
	GW::CORE::GEventGenerator _eventPusher,
	std::shared_ptr<Level_Data> _levelData)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	eventPusher = _eventPusher;
	levelData = _levelData;

	// destroy any bullets that have the CollidedWith relationship
	game->system<Enemy, Health, Position>("Enemy System")
		.each([this](flecs::entity e, Enemy, Health& h, Position& p) {
		// if you have no health left be destroyed
		if (e.get<Health>()->value <= 0) {
			// play explode sound
			e.destruct();
			ESG::PLAY_EVENT_DATA x;
			GW::GEvent explode;
			explode.Write(ESG::PLAY_EVENT::ENEMY_DESTROYED, x);
			eventPusher.Push(explode);
		}
		ModelTransform* edit = e.get_mut<ModelTransform>();
		//moveRight = true;

		if (moveRight)
		{
			GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ 0, 0, 1, 1 }, edit->matrix);
			levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
			if (edit->matrix.row4.z >= 40)
			{
				moveRight = false;
				GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ 0, -5, 0, 1 }, edit->matrix);
				levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
			}
		}
		else if (!moveRight)
		{
			GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ 0, 0, -1, 1 }, edit->matrix);
			levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
			if (edit->matrix.row4.z <= -40)
			{
				moveRight = true;
				GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ 0, -5, 0, 1 }, edit->matrix);
				levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
			}
		}

		std::cout << "Moving " << edit->matrix.row4.x << " " << edit->matrix.row4.y << " " << edit->matrix.row4.z << std::endl;

		if (edit->matrix.row4.y <= 30)
		{
			e.destruct();
			flecs::entity playerDeath;
			RetreivePrefab("Death", playerDeath);
			GW::AUDIO::GSound death = *playerDeath.get<GW::AUDIO::GSound>();
			death.Play();
			//GW::AUDIO::GSound death = = 
			std::cout << "Player Dies...You Lose";
		}
		p.value = { 0, 0 };

		//FireLasersEnemy(e.world(), p);
		});

	return true;
}

// Free any resources used to run this system
bool ESG::EnemyLogic::Shutdown()
{
	game->entity("Enemy System").destruct();
	// invalidate the shared pointers
	game.reset();
	gameConfig.reset();
	return true;
}

// Toggle if a system's Logic is actively running
bool ESG::EnemyLogic::Activate(bool runSystem)
{
	if (runSystem) {
		game->entity("Enemy System").enable();
	}
	else {
		game->entity("Enemy System").disable();
	}
	return false;
}

bool ESG::EnemyLogic::FireLasersEnemy(flecs::world& stage, Position origin)
{
	// Grab laser prefab
	flecs::entity bullet;
	RetreivePrefab("Lazer Bullet", bullet);

	// Laser start shoot position
	origin.value.x -= 0.05f;
	auto laserLeft = stage.entity().is_a(bullet)
		.set<Position>(origin);
	origin.value.x += 0.1f;
	auto laserRight = stage.entity().is_a(bullet)
		.set<Position>(origin);

	// Tank shot

	return true;
}