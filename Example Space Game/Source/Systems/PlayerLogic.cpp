#include "PlayerLogic.h"
#include "../Components/Identification.h"
#include "../Components/Physics.h"
#include "../Components/Visuals.h"
#include "../Components/Gameplay.h"
#include "../Components/Components.h"
#include "../Entities/Prefabs.h"
#include "../Events/Playevents.h"

using namespace GA; // Example Space Game
using namespace GW::INPUT; // input libs
using namespace GW::AUDIO; // audio libs

// Connects logic to traverse any players and allow a controller to manipulate them
bool GA::PlayerLogic::Init(std::shared_ptr<flecs::world> _game, 
							std::weak_ptr<const GameConfig> _gameConfig, 
							GW::INPUT::GInput _immediateInput, 
							GW::INPUT::GBufferedInput _bufferedInput, 
							GW::INPUT::GController _controllerInput,
							GW::AUDIO::GAudio _audioEngine,
							GW::CORE::GEventGenerator _eventPusher,
							std::shared_ptr<Level_Data> _levelData, std::shared_ptr<int> _currentLevel,
							std::shared_ptr<bool> _levelChange, std::shared_ptr<bool> _youWin, std::shared_ptr<bool> _youLose)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	immediateInput = _immediateInput;
	bufferedInput = _bufferedInput;
	controllerInput =	_controllerInput;
	audioEngine = _audioEngine;
	levelData = _levelData;
	currentLevel = _currentLevel;
	levelChange = _levelChange;
	youWin = _youWin;
	youLose = _youLose;
	// Init any helper systems required for this task
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	int width = (*readCfg).at("Window").at("width").as<int>();
	float speed = (*readCfg).at("Player").at("speed").as<float>();
	chargeTime = (*readCfg).at("Player").at("chargeTime").as<float>();
	// add logic for updating players
	playerSystem = game->system<Player, Position, ControllerID>("Player System")
		.iter([this, speed](flecs::iter it, Player*, Position* p, ControllerID* c) {

		for (auto i : it) 
		{
			// left-right movement
			float xaxis = 0, input = 0;
			// Use the controller/keyboard to move the player around the screen			
			if (c[i].index == 0) { // enable keyboard controls for player 1
				immediateInput.GetState(G_KEY_LEFT, input); xaxis -= input;
				immediateInput.GetState(G_KEY_RIGHT, input); xaxis += input;
			}
			// grab left-thumb stick
			controllerInput.GetState(c[i].index, G_LX_AXIS, input); xaxis += input;
			controllerInput.GetState(c[i].index, G_DPAD_LEFT_BTN, input); xaxis -= input;
			controllerInput.GetState(c[i].index, G_DPAD_RIGHT_BTN, input); xaxis += input;
			xaxis = G_LARGER(xaxis, -1);// cap right motion
			xaxis = G_SMALLER(xaxis, 1);// cap left motion

			// apply movement
			p[i].value.x += xaxis * it.delta_time() * speed;
			// limit the player to stay within -1 to +1 NDC
			p[i].value.x = G_LARGER(p[i].value.x, -0.4f);
			p[i].value.x = G_SMALLER(p[i].value.x, +0.4f);

			ModelTransform* edit = it.entity(i).get_mut<ModelTransform>();

			if (edit->matrix.row4.z > -42 && edit->matrix.row4.z < +42)
			{
				GW::MATH::GMatrix::TranslateLocalF(edit->matrix, GW::MATH::GVECTORF{ -p[i].value.x, p[i].value.y, 0, 1 }, edit->matrix); 
				if (edit->matrix.row4.z < -40) 
					edit->matrix.row4.z = -40; 
				else if (edit->matrix.row4.z > 40) 
					edit->matrix.row4.z = 40; 

				levelData->levelTransforms[edit->rendererIndex] = edit->matrix; 
			}

			flecs::entity bull;
			RetreivePrefab("Lazer Bullet", bull);
			ModelTransform* bullet = bull.get_mut<ModelTransform>();
			bullet->matrix.row4.z = edit->matrix.row4.z;
			std::cout << "bulletZ: " << bullet->matrix.row4.z << " playerZ: " << edit->matrix.row4.z << '\n';

			if (bull.has<BulletTest>())
			{
				// fire weapon if they are in a firing state
				if (it.entity(i).has<Firing>()) {
					Position offset = p[i];
					offset.value.x = p[i].value.x;
					offset.value.y = 0.05;
					FireLasers(it.entity(i).world(), offset);



					if (offset.value.y >= 50)
					{
						it.entity(i).remove<Firing>();
						std::cout << "Bullet Moved \n";
					}
				}
			}
			p[i].value.x = 0; 
			p[i].value.y = 0;		
		}
		// process any cached button events after the loop (happens multiple times per frame)
		ProcessInputEvents(it.world());
	});

	// Create an event cache for when the spacebar/'A' button is pressed
	pressEvents.Create(Max_Frame_Events); // even 32 is probably overkill for one frame
		
	// register for keyboard and controller events
	bufferedInput.Register(pressEvents);
	controllerInput.Register(pressEvents);

	// create the on explode handler
	onExplode.Create([this](const GW::GEvent& e) {
		GA::PLAY_EVENT event; GA::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData) && event == GA::PLAY_EVENT::ENEMY_DESTROYED) {
			// only in here if event matches
			std::cout << "Enemy Was Destroyed!\n";
		}
		});


	nextLevel.Create([this](const GW::GEvent& e) {
		GA::PLAY_EVENT event; GA::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData) && event == GA::PLAY_EVENT::NEXT_LEVEL) {
			// only in here if event matches
			GW::SYSTEM::GLog log;
			++(*currentLevel);
			(*levelChange) = true;
		}
		});

	resetLevel.Create([this](const GW::GEvent& e) {
		GA::PLAY_EVENT event; GA::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData) && event == GA::PLAY_EVENT::RESET_LEVEL) {
			// only in here if event matches
			GW::SYSTEM::GLog log;
			(*currentLevel) = 1;
			(*levelChange) == true;
		}
		});

	youWon.Create([this](const GW::GEvent& e) {
		GA::PLAY_EVENT event; GA::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData) && event == GA::PLAY_EVENT::WIN) {
			// only in here if event matches
			*youWin = true;
		}
		});

	youLost.Create([this](const GW::GEvent& e) {
		GA::PLAY_EVENT event; GA::PLAY_EVENT_DATA eventData;
		if (+e.Read(event, eventData) && event == GA::PLAY_EVENT::LOSE) {
			// only in here if event matches
			*youLose = true;
		}
		});

	_eventPusher.Register(onExplode);
	_eventPusher.Register(nextLevel);
	_eventPusher.Register(resetLevel);
	_eventPusher.Register(youWon);
	_eventPusher.Register(youLost);
	return true;
}

// Free any resources used to run this system
bool GA::PlayerLogic::Shutdown()
{
	playerSystem.destruct();
	game.reset();
	gameConfig.reset();

	return true;
}

// Toggle if a system's Logic is actively running
bool GA::PlayerLogic::Activate(bool runSystem)
{
	if (playerSystem.is_alive()) {
		(runSystem) ? 
			playerSystem.enable() 
			: playerSystem.disable();
		return true;
	}
	return false;
}

bool GA::PlayerLogic::ProcessInputEvents(flecs::world& stage)
{
	// pull any waiting events from the event cache and process them
	GW::GEvent event;
	while (+pressEvents.Pop(event)) {
		bool fire = false;
		GController::Events controller;
		GController::EVENT_DATA c_data;
		GBufferedInput::Events keyboard;
		GBufferedInput::EVENT_DATA k_data;
		// these will only happen when needed
		if (+event.Read(keyboard, k_data)) {
			if (keyboard == GBufferedInput::Events::KEYPRESSED) {
				if (k_data.data == G_KEY_SPACE) {
					fire = true;
					//chargeStart = stage.time();
				}
				if (k_data.data == G_KEY_0)
				{
					//allow shield to turn off and on
					///stage.entity("Player").add<
				}
			}
			//if (keyboard == GBufferedInput::Events::KEYRELEASED) {
			//	if (k_data.data == G_KEY_SPACE) {
			//		//chargeEnd = stage.time();
			//		if (chargeEnd - chargeStart >= chargeTime) {
			//			fire = true;
			//		}
			//	}
			//}
		}
		else if (+event.Read(controller, c_data)) {
			if (controller == GController::Events::CONTROLLERBUTTONVALUECHANGED) {
				if (c_data.inputValue > 0 && c_data.inputCode == G_SOUTH_BTN)
					fire = true;
			}
		}
		if (fire) {
			// grab player one and set them to a firing state
			stage.entity("Player").add<Firing>();
		}
	}
	return true;
}

// play sound and launch two laser rounds
bool GA::PlayerLogic::FireLasers(flecs::world& stage, Position origin)
{
	// Grab the prefab for a laser round
	flecs::entity bullet;
	flecs::entity bullet2;
	RetreivePrefab("Lazer Bullet", bullet);
	RetreivePrefab("Lazer Bullet2", bullet2);

	flecs::entity bulletRay[] = { bullet, bullet2 };

	//if (bulletRay[1].is_valid())
	//{
		bullet.add<BulletTest>();

		ModelTransform* bulletT = bullet.get_mut<ModelTransform>();

		/*if (bulletT->matrix.row4.y >= 200)
		{
			std::cout << "Bullet destruct \n";
			return false;
		}
		else
		{*/
		GW::MATH::GMatrix::TranslateGlobalF(bulletT->matrix, GW::MATH::GVECTORF{ origin.value.x, 5, 0, 1 }, bulletT->matrix);
		levelData->levelTransforms[bulletT->rendererIndex] = bulletT->matrix;
		origin.value.x = bulletT->matrix.row4.x;
		origin.value.y = bulletT->matrix.row4.y;
		std::cout << "Bullet Matrix changes: " << "x: " << bulletT->matrix.row4.x << " y: " << bulletT->matrix.row4.y << " orginY: " << origin.value.y << '\n';
	//}
	//else
	//{
	//	bulletRay[2].add<BulletTest>();

	//	ModelTransform* bulletT = bulletRay[2].get_mut<ModelTransform>();

	//	/*if (bulletT->matrix.row4.y >= 200)
	//	{
	//		std::cout << "Bullet destruct \n";
	//		return false;
	//	}
	//	else
	//	{*/
	//	GW::MATH::GMatrix::TranslateGlobalF(bulletT->matrix, GW::MATH::GVECTORF{ origin.value.x, 5, 0, 1 }, bulletT->matrix);
	//	levelData->levelTransforms[bulletT->rendererIndex] = bulletT->matrix;
	//	std::cout << "Bullet Matrix changes: " << "x: " << bulletT->matrix.row4.x << " y: " << bulletT->matrix.row4.y << " z: " << bulletT->matrix.row4.z << '\n';
	//}
	//}

	//origin.value.x -= 0.05f;
	//auto laserLeft = stage.entity().is_a(bullet)
	//	.set<Position>(origin);
	/*origin.value.x += 0.1f;*/
	/*auto laserRight = stage.entity().is_a(bullet)
		.set<Position>(origin);
	laserRight.add<BulletTest>();*/

	//ModelTransform* edit = laserRight.get_mut<ModelTransform>();
	//edit = original;
	// if this shot is charged
	//if (chargeEnd - chargeStart >= chargeTime) {
	//	chargeEnd = chargeStart;
	//	/*laserLeft.set<ChargedShot>({ 2 })
	//		.set<Material>({1,0,0});*/
	//	laserRight.set<ChargedShot>({ 2 })
	//		.set<Material>({ 1,0,0 });
	//}
	
	//origin.value.y += 1.0f;

	//GW::MATH::GMatrix::TranslateGlobalF(edit->matrix, GW::MATH::GVECTORF{ origin.value.x, origin.value.y, 0, 1 }, edit->matrix);
	//levelData->levelTransforms[edit->rendererIndex] = edit->matrix;
	//printf("%f %f \n", edit->matrix.row4.x, edit->matrix.row4.y);

	// play the sound of the Lazer prefab
	GW::AUDIO::GSound shoot = *bullet.get<GW::AUDIO::GSound>();
	shoot.Play();
	
	return true;
}