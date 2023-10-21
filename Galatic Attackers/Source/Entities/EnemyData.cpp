#include "EnemyData.h"
#include "../Components/Identification.h"
#include "../Components/visuals.h"
#include "../Components/Physics.h"
#include "../Entities/Prefabs.h"
#include "../Components/Gameplay.h"
#include "../Components/Components.h"

bool GA::EnemyData::Load(	std::shared_ptr<flecs::world> _game,
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
	GW::MATH2D::GMatrix2D::Rotate2F(GW::MATH2D::GIdentityMatrix2F, 
		G_DEGREE_TO_RADIAN_F(angle), world);
	GW::MATH2D::GMatrix2D::Scale2F(world,
		GW::MATH2D::GVECTOR2F{ xscale, yscale }, world);
	
	// add prefab to ECS
	//auto enemyPrefab = _game->lookup("Spaceship5");
	//auto enemyPrefab = _game->entity("Spaceship5")
		//auto spaceship = _game->entity("Spaceship5");
		auto enemyPrefab = _game->prefab("Spaceship5")
		// .set<> in a prefab means components are shared (instanced)
		//.set(spaceship)
			.set<Material>({ red, green, blue })
			.set<Orientation>({ world })
			.set<ModelBoundary*>(_game->prefab("Spaceship5").get_mut<ModelBoundary>())
		// .override<> ensures a component is unique to each entity created from a prefab
			.set_override<Health>({ health })
			.override<Acceleration>()
			.override<Velocity>()
			.override<Position>()
			.override<Enemy>() // Tag this prefab as an enemy (for queries/systems)
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with
		if (enemyPrefab.has<Collidable>())
		{
			std::cout << "ship got bound: " << enemyPrefab.get_mut<ModelBoundary>() << '\n';
			std::cout << "ship orig bound: " << _game->prefab("Spaceship5").get_mut<ModelBoundary>() << '\n';
		}

		auto enemyPrefab1 = _game->prefab("Spaceship5.001")
			// .set<> in a prefab means components are shared (instanced)
			//.set(spaceship)
			//.set<Material>({ red, green, blue })
			//.set<Orientation>({ world })
			// .override<> ensures a component is unique to each entity created from a prefab
			.set_override<Health>({ health })
			//.override<Acceleration>()
			//.override<Velocity>()
			//.override<Position>()
			.override<Enemy>() // Tag this prefab as an enemy (for queries/systems)
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with
		auto enemyPrefab2 = _game->prefab("Spaceship5.002")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab3 = _game->prefab("Spaceship5.003")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab4 = _game->prefab("Spaceship5.004")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab5 = _game->prefab("Spaceship5.005")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab6 = _game->prefab("Spaceship5.006")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab7 = _game->prefab("Spaceship5.007")
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
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto enemyPrefab8 = _game->prefab("Spaceship5.008")
			 //.set<> in a prefab means components are shared (instanced)
			//.set(spaceship)
			.set<Material>({ red, green, blue })
			.set<Orientation>({ world })
			 //.override<> ensures a component is unique to each entity created from a prefab
			.set_override<Health>({ health })
			.override<Acceleration>()
			.override<Velocity>()
			.override<Position>()
			.override<Enemy>() // Tag this prefab as an enemy (for queries/systems)
			//.override<PrefabEnemy>() // Tag this prefab as an enemy (for queries/systems)
			.override<Collidable>(); // can be collided with

		auto a = _game->lookup("Spaceship5.008");
		if (a.is_valid()) {
			//a.add<Health>(health);
			a.override<Enemy>();
			a.set_override<Health>({ health });
			a.add<Orientation>();
			a.add<Position>();
			a.add<Collidable>();
		}
		auto b = _game->lookup("Spaceship5.007");
		if (b.is_valid()) {
			b.add<Orientation>();
			b.add<Position>();
			b.add<Collidable>();
		}
		auto c = _game->lookup("Spaceship5.006");
		if (c.is_valid()) {
			c.add<Orientation>();
			c.add<Position>();
			c.add<Collidable>();
		}
		auto d = _game->lookup("Spaceship5.005");
		if (d.is_valid()) {
			d.add<Orientation>();
			d.add<Position>();
			d.add<Collidable>();
		}
		auto e = _game->lookup("Spaceship5.004");
		if (e.is_valid()) {
			e.add<Orientation>();
			e.add<Position>();
			e.add<Collidable>();
		}
		auto f = _game->lookup("Spaceship5.003");
		if (f.is_valid()) {
			f.add<Orientation>();
			f.add<Position>();
			f.add<Collidable>();
		}
		auto g = _game->lookup("Spaceship5.002");
		if (g.is_valid()) {
			g.add<Orientation>();
			g.add<Position>();
			g.add<Collidable>();
		}
		auto h = _game->lookup("Spaceship5.001");
		if (h.is_valid()) {
			h.add<Orientation>();
			h.add<Position>();
			h.add<Collidable>();
		}
		auto i = _game->lookup("Spaceship5");
		if (i.is_valid()) {
			i.add<Orientation>();
			i.add<Position>();
			i.add<Collidable>();
		}
		 

	// register this prefab by name so other systems can use it
	RegisterPrefab("Enemy Type1", enemyPrefab);
	RegisterPrefab("Enemy Type2", enemyPrefab1);
	RegisterPrefab("Enemy Type3", enemyPrefab2);
	RegisterPrefab("Enemy Type4", enemyPrefab3);
	RegisterPrefab("Enemy Type5", enemyPrefab4);
	RegisterPrefab("Enemy Type6", enemyPrefab5);
	RegisterPrefab("Enemy Type7", enemyPrefab6);
	RegisterPrefab("Enemy Type8", enemyPrefab7);
	RegisterPrefab("Enemy Type9", enemyPrefab8);

	return true;
}

bool GA::EnemyData::Unload(std::shared_ptr<flecs::world> _game)
{
	// remove all bullets and their prefabs
	_game->defer_begin(); // required when removing while iterating!
	_game->each([](flecs::entity e, Enemy&) {
		e.destruct(); // destroy this entitiy (happens at frame end)
	});
	_game->defer_end(); // required when removing while iterating!

	return true;
}