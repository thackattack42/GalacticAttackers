#include "EnemyData.h"
#include "../Components/Identification.h"
#include "../Components/visuals.h"
#include "../Components/Physics.h"
#include "../Entities/Prefabs.h"
#include "../Components/Gameplay.h"

bool ESG::EnemyData::Load(	std::shared_ptr<flecs::world> _game,
							std::weak_ptr<const GameConfig> _gameConfig,
							GW::AUDIO::GAudio _audioEngine)
{
	// Grab init settings for players
	std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();

	// Create prefab for lazer weapon
	// color
	float red = (*readCfg).at("Enemy1").at("red").as<float>();
	float green = (*readCfg).at("Enemy1").at("green").as<float>();
	float blue = (*readCfg).at("Enemy1").at("blue").as<float>();
	// other attributes
	float xscale = (*readCfg).at("Enemy1").at("xscale").as<float>();
	float yscale = (*readCfg).at("Enemy1").at("yscale").as<float>();
	float angle = (*readCfg).at("Enemy1").at("angle").as<float>();
	int health = (*readCfg).at("Enemy1").at("health").as<int>();
	
	// default projectile orientation & scale
	GW::MATH2D::GMATRIX2F world;
	GW::MATH2D::GMATRIX2F world2;
	GW::MATH2D::GMATRIX2F world3;
	GW::MATH2D::GMatrix2D::Rotate2F(GW::MATH2D::GIdentityMatrix2F, 
		G_DEGREE_TO_RADIAN_F(angle), world);
	GW::MATH2D::GMatrix2D::Rotate2F(GW::MATH2D::GIdentityMatrix2F, 
		G_DEGREE_TO_RADIAN_F(angle), world2);
	GW::MATH2D::GMatrix2D::Rotate2F(GW::MATH2D::GIdentityMatrix2F, 
		G_DEGREE_TO_RADIAN_F(angle), world3);
	GW::MATH2D::GMatrix2D::Scale2F(world,
		GW::MATH2D::GVECTOR2F{ xscale, yscale }, world);
	GW::MATH2D::GMatrix2D::Scale2F(world2,
		GW::MATH2D::GVECTOR2F{ xscale, yscale }, world2);
	GW::MATH2D::GMatrix2D::Scale2F(world3,
		GW::MATH2D::GVECTOR2F{ xscale, yscale }, world3);

	
	// add prefab to ECS
	//auto enemyPrefab = _game->lookup("Spaceship5");
	//auto enemyPrefab = _game->entity("Spaceship5")
		//auto spaceship = _game->entity("Spaceship5");
	auto enemyPrefab = _game->prefab("Spaceship5");
		// .set<> in a prefab means components are shared (instanced)
		//.set(spaceship)
	if (enemyPrefab.is_valid()) 
	{
		enemyPrefab.set<Material>({ red, green, blue });
		enemyPrefab.set<Orientation>({ world });
		// .override<> ensures a component is unique to each entity created from a prefab
		enemyPrefab.set_override<Health>({ health });
		enemyPrefab.override<Acceleration>();
		enemyPrefab.override<Velocity>();
		enemyPrefab.override<Position>();
		enemyPrefab.override<Enemy>(); // Tag this prefab as an enemy (for queries/systems)
		enemyPrefab.override<Collidable>(); // can be collided with
	}
	auto enemyPrefab2 = _game->prefab("Spaceship2");
	if (enemyPrefab2.is_valid())
	{
		enemyPrefab2.set<Material>({ red, green, blue });
		enemyPrefab2.set<Orientation>({ world2 });
		enemyPrefab2.set_override<Health>({ health });
		enemyPrefab2.override<Acceleration>();
		enemyPrefab2.override<Velocity>();
		enemyPrefab2.override<Position>(); 
		enemyPrefab2.override<Enemy>(); 
		enemyPrefab2.override<Collidable>();
	}
	
	auto enemyPrefab3 = _game->prefab("Spaceship4");
	if (enemyPrefab3.is_valid())
	{
		enemyPrefab3.set<Material>({ red, green, blue });
		enemyPrefab3.set<Orientation>({ world3 });
		enemyPrefab3.set_override<Health>({ health });
		enemyPrefab3.override<Acceleration>();
		enemyPrefab3.override<Velocity>();
		enemyPrefab3.override<Position>();
		enemyPrefab3.override<Enemy>();
		enemyPrefab3.override<Collidable>();
	}

	// register this prefab by name so other systems can use it
	RegisterPrefab("Spaceship5", enemyPrefab);
	RegisterPrefab("Spaceship2", enemyPrefab2);
	RegisterPrefab("Spaceship4", enemyPrefab3);
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
