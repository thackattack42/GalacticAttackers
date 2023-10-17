#include "EnemyData.h"
#include "../Components/Identification.h"
#include "../Components/visuals.h"
#include "../Components/Physics.h"
#include "../Entities/Prefabs.h"
#include "../Components/Gameplay.h"
#include "../Components/Components.h"


bool ESG::EnemyData::Load(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine, std::shared_ptr<Level_Data> _levelData)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
	levelData = _levelData;
	// Create prefab for lazer weapon
	// color
	float red = (*readCfg).at("Enemy1").at("red").as<float>();
	float green = (*readCfg).at("Enemy1").at("green").as<float>();
	float blue = (*readCfg).at("Enemy1").at("blue").as<float>();
	
	float red2 = (*readCfg).at("Enemy2").at("red").as<float>();
	float green2 = (*readCfg).at("Enemy2").at("green").as<float>();
	float blue2 = (*readCfg).at("Enemy2").at("blue").as<float>();

	float red3 = (*readCfg).at("Enemy3").at("red").as<float>();
	float green3 = (*readCfg).at("Enemy3").at("green").as<float>();
	float blue3 = (*readCfg).at("Enemy3").at("blue").as<float>();
	// other attributes
	float xscale = (*readCfg).at("Enemy1").at("xscale").as<float>();
	float yscale = (*readCfg).at("Enemy1").at("yscale").as<float>();
	float angle = (*readCfg).at("Enemy1").at("angle").as<float>();
	int health = (*readCfg).at("Enemy1").at("health").as<int>();

	float xscale2 = (*readCfg).at("Enemy2").at("xscale").as<float>();
	float yscale2 = (*readCfg).at("Enemy2").at("yscale").as<float>();
	float angle2 = (*readCfg).at("Enemy2").at("angle").as<float>();
	int health2 = (*readCfg).at("Enemy2").at("health").as<int>();

	float xscale3 = (*readCfg).at("Enemy3").at("xscale").as<float>();
	float yscale3 = (*readCfg).at("Enemy3").at("yscale").as<float>();
	float angle3 = (*readCfg).at("Enemy3").at("angle").as<float>();
	int health3 = (*readCfg).at("Enemy3").at("health").as<int>();
	
	// default projectile orientation & scale
	GW::MATH2D::GMATRIX2F world;
	GW::MATH2D::GMatrix2D::Rotate2F(GW::MATH2D::GIdentityMatrix2F, 
		G_DEGREE_TO_RADIAN_F(angle), world);
	GW::MATH2D::GMatrix2D::Scale2F(world,
		GW::MATH2D::GVECTOR2F{ xscale, yscale }, world);
	
	// add prefab to ECS
	
	std::shared_ptr<flecs::world> newWorld = std::make_shared<flecs::world>();
	

		auto enemyPrefab = newWorld->entity("Spaceship5")
		// .set<> in a prefab means components are shared (instanced)
		//.set(spaceship)
			
			.set<Material>({ red, green, blue })
			.set<Orientation>({ world })
		// .override<> ensures a component is unique to each entity created from a prefab
			.set_override<Health>({ health })
			.override<Acceleration>()
			.override<Velocity>()
			.override<Position>()
			.override<Enemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with
		std::shared_ptr<flecs::world> newWorld = std::make_shared<flecs::world>();


		if (enemyPrefab.is_valid())
		{
			ModelTransform* edit = newWorld->entity(enemyPrefab).get_mut<ModelTransform>();
			if (edit)
			{
				//GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ , -p.value.y, 0, 1 }, edit->matrix);
				levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
				std::cout << "X value: " << p.value.x << "___Y value: " << p.value.y << "\n";
				std::cout << "X valueEdit: " << edit->matrix.row4.x << "___Y valueEdit: " << edit->matrix.row4.y << "\n";
			}
		}

		 auto enemyPrefab2 = _game->prefab("Spaceship2") 
			.set<Material>({ red2, green2, blue2 }) 
			.set<Orientation>({ world }) 
			.set_override<Health>({ health })
			.override<Acceleration>() 
			.override<Velocity>() 
			.override<Position>()
			.override<Enemy>() 
			.override<Collidable>();
	
		 auto enemyPrefab3 = _game->prefab("Spaceship4")
			 .set<Material>({ red3, green3, blue3 })
			 .set<Orientation>({ world })
			 .set_override<Health>({ health })
			 .override<Acceleration>()
			 .override<Velocity>()
			 .override<Position>()
			 .override<Enemy>()
			 .override<Collidable>();

	// register this prefab by name so other systems can use it
	RegisterPrefab("Enemy Type1", enemyPrefab);
	RegisterPrefab("Enemy Type2", enemyPrefab2);
	RegisterPrefab("Enemy Type3", enemyPrefab3);
	//RegisterPrefab("Enemy Type2", enemyPrefab2);

	return true;
}

bool ESG::EnemyData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all bullets and their prefabs
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Enemy&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
	});
	_game->defer_end(); // required when removing while iterating!

	return true;
}
