#include "Application.h"
#include "Components\Identification.h"
#include "Components\Visuals.h"
// open some Gateware namespaces for conveinence 
// NEVER do this in a header file!
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
using namespace ESG;

bool Application::Init()
{
	eventPusher.Create();

	// load all game settigns
	gameConfig = std::make_shared<GameConfig>(); 
	// create the ECS system
	game = std::make_shared<flecs::world>(); 
	levelData = std::make_shared<Level_Data>();
	//for changing level data				level positioning		level obj/mtl
	switch (currentLevel)
	{
	case 1:
		level = gameConfig->at("LevelFile").at("levelone").as<std::string>();
		break;
	case 2:
		level = gameConfig->at("LevelFile").at("leveltwo").as<std::string>();
		break;
	case 3:
		level = gameConfig->at("LevelFile").at("levelthree").as<std::string>();
		break;
	default:
		level = gameConfig->at("LevelFile").at("levelone").as<std::string>();
		break;
	}
	
	models = gameConfig->at("ModelFolder").at("models").as<std::string>();

	if (levelData->LoadLevel(level.c_str(), models.c_str(), log) == false)
		return false;

	for (auto& i : levelData->blenderObjects) {
		// create entity with same name as blender object
		auto ent = game->entity(i.blendername);
		ent.set<BlenderName>({ i.blendername });
		ent.set<ModelBoundary>({
			levelData->levelColliders[levelData->levelModels[i.modelIndex].colliderIndex] });
		ent.set<ModelTransform>({
			levelData->levelTransforms[i.transformIndex], i.transformIndex });
		ent.set<Material>({ 1, 1, 1 });
		ent.add<RenderingSystem>();
		ent.set<Instance>({ levelData->levelInstances[i.modelIndex] });
		ent.set<Object>({ levelData->levelModels[levelData->levelInstances[i.modelIndex].modelIndex]});

	}
	// for now just print objects added to FLECS
	auto f = game->filter<BlenderName, ModelTransform>();
	/*f.each([&log](BlenderName& n, ModelTransform& t) {
		std::string obj = "FLECS Entity ";
		obj += n.name + " located at X " + std::to_string(t.matrix.row4.x) +
			" Y " + std::to_string(t.matrix.row4.y) + " Z " + std::to_string(t.matrix.row4.z);
		log.LogCategorized("GAMEPLAY", obj.c_str());
		});*/

	// init all other systems
	if (InitWindow() == false) 
		return false;
	if (InitInput() == false)
		return false;
	if (InitAudio() == false)
		return false;
	if (InitGraphics() == false)
		return false;
	if (InitEntities() == false)
		return false;
	if (InitSystems() == false)
		return false;
	return true;
}

bool Application::Run() 
{
	//VkClearValue clrAndDepth[2];
	//clrAndDepth[0].color = { {0, 0, 0, 1} };
	//clrAndDepth[1].depthStencil = { 1.0f, 0u };
	// grab vsync selection
	bool vsync = gameConfig->at("Window").at("vsync").as<bool>();
	float color[4] = { 0,0,0,0 };
	// set background color from settings
	const char* channels[] = { "red", "green", "blue" };
	for (int i = 0; i < std::size(channels); ++i) {
		/*clrAndDepth[0].color.float32[i] =*/
		color[i] = 
			gameConfig->at("BackGroundColor").at(channels[i]).as<float>();
	}
	// create an event handler to see if the window was closed early
	bool winClosed = false;
	GW::CORE::GEventResponder winHandler;
	winHandler.Create([&winClosed](GW::GEvent e) {
		GW::SYSTEM::GWindow::Events ev;
		if (+e.Read(ev) && ev == GW::SYSTEM::GWindow::Events::DESTROY)
			winClosed = true;
	});	
	window.Register(winHandler);
	while (+window.ProcessWindowEvents())
	{
		if (winClosed == true)
			return true;
		//if (+vulkan.StartFrame(2, clrAndDepth))
		//{
		//	if (GameLoop() == false) {
		//		vulkan.EndFrame(vsync);
		//		return false;
		//	}
		//	if (-vulkan.EndFrame(vsync)) {
		//		// failing EndFrame is not always a critical error, see the GW docs for specifics
		//	}
		//}
		//else
		//	return false;
		IDXGISwapChain* swap;
		DXGI_SWAP_CHAIN_DESC chain;
		ZeroMemory(&chain, sizeof(DXGI_SWAP_CHAIN_DESC));
		chain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		ID3D11DeviceContext* con;
		ID3D11RenderTargetView* view;
		ID3D11DepthStencilView* depth;
		
		
		if (+d3d11.GetImmediateContext((void**)&con) &&
			+d3d11.GetRenderTargetView((void**)&view) &&
			+d3d11.GetDepthStencilView((void**)&depth) &&
			+d3d11.GetSwapchain((void**)&swap))
		{
			con->ClearRenderTargetView(view, color);
			con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
			//renderer.UpdateWindowSize();
			//renderer.PlayerSwap();
			//renderer.UpdateCamera();
			GameLoop();
			//renderer.LevelSwitch(log);
			//renderer.Render();
			swap->Present(1, 0);
			// release incremented COM reference counts
			swap->SetFullscreenState(FALSE, NULL);
			swap->Release();
			view->Release();
			depth->Release();
			con->Release();
		}

	}
	return true;
}

bool Application::Shutdown() 
{
	// disconnect systems from global ECS
	if (playerSystem.Shutdown() == false)
		return false;
	if (levelSystem.Shutdown() == false)
		return false;
	if (d3dRenderingSystem.Shutdown() == false)
		return false;
	if (physicsSystem.Shutdown() == false)
		return false;
	if (bulletSystem.Shutdown() == false)
		return false;
	if (enemySystem.Shutdown() == false)
		return false;

	return true;
}

bool Application::InitWindow()
{
	// grab settings
	int width = gameConfig->at("Window").at("width").as<int>();
	int height = gameConfig->at("Window").at("height").as<int>();
	int xstart = gameConfig->at("Window").at("xstart").as<int>();
	int ystart = gameConfig->at("Window").at("ystart").as<int>();
	std::string title = gameConfig->at("Window").at("title").as<std::string>();
	// open window
	if (+window.Create(xstart, ystart, width, height, GWindowStyle::WINDOWEDBORDERED) &&
		+window.SetWindowName(title.c_str())) {
		return true;
	}
	return false;
}

bool Application::InitInput()
{
	if (-gamePads.Create())
		return false;
	if (-immediateInput.Create(window))
		return false;
	if (-bufferedInput.Create(window))
		return false;
	return true;
}

bool Application::InitAudio()
{
	if (-audioEngine.Create())
		return false;
	return true;
}

bool Application::InitGraphics()
{
//#ifndef NDEBUG
//	const char* debugLayers[] = {
//		"VK_LAYER_KHRONOS_validation", // standard validation layer
//		//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
//	};
//	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
//		sizeof(debugLayers) / sizeof(debugLayers[0]),
//		debugLayers, 0, nullptr, 0, nullptr, false))
//		return true;
//#else
//	if (+vulkan.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
//		return true;
//#endif

	if (+d3d11.Create(window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		return true;

	return false;
}

bool Application::InitEntities()
{
	// Load bullet prefabs
	if (weapons.Load(game, gameConfig, audioEngine) == false)
		return false;
	// Load the player entities
	if (players.Load(game, gameConfig) == false)
		return false;
	// Load the enemy entities
	if (enemies.Load(game, gameConfig, audioEngine) == false)
		return false;

	return true;
}

bool Application::InitSystems()
{
	// connect systems to global ECS
	if (playerSystem.Init(	game, gameConfig, immediateInput, bufferedInput, 
							gamePads, audioEngine, eventPusher) == false)
		return false;
	if (levelSystem.Init(game, gameConfig, audioEngine) == false)
		return false;
	if (d3dRenderingSystem.Init(game, gameConfig, d3d11, window, levelData) == false)
		return false;
	if (physicsSystem.Init(game, gameConfig) == false)
		return false;
	if (bulletSystem.Init(game, gameConfig) == false)
		return false;
	if (enemySystem.Init(game, gameConfig, eventPusher) == false)
		return false;

	return true;
}

bool Application::GameLoop()
{
	// compute delta time and pass to the ECS system
	static auto start = std::chrono::steady_clock::now();
	double elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();
	start = std::chrono::steady_clock::now();
	// let the ECS system run
	return game->progress(static_cast<float>(elapsed)); 
}
