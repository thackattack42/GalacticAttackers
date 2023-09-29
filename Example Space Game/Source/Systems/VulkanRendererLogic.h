// The rendering system is responsible for drawing all objects
#ifndef VULKANRENDERERLOGIC_H
#define VULKANRENDERERLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"

// example space game (avoid name collisions)
namespace ESG
{
	class D3DRendererLogic
	{
		// shared connection to the main ECS engine
		std::shared_ptr<flecs::world> game;
		// non-ownership handle to configuration settings
		std::weak_ptr<const GameConfig> gameConfig;
		// handle to our running ECS systems
		flecs::system startDraw;
		flecs::system updateDraw;
		flecs::system completeDraw;
		// Used to query screen dimensions
		GW::SYSTEM::GWindow window;
		// Directx11 resources used for rendering
		GW::GRAPHICS::GDirectX11Surface direct11;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	    indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;
		// used to trigger clean up of vulkan resources
		GW::CORE::GEventReceiver shutdown;
	public:
		// attach the required logic to the ECS 
		bool Init(	std::shared_ptr<flecs::world> _game,
					std::weak_ptr<const GameConfig> _gameConfig,
					GW::GRAPHICS::GDirectX11Surface _direct11,
					GW::SYSTEM::GWindow _window);
		// control if the system is actively running
		bool Activate(bool runSystem);
		// release any resources allocated by the system
		bool Shutdown();
	private:
		struct PipelineHandles
		{
			ID3D11DeviceContext* context;
			ID3D11RenderTargetView* targetView;
			ID3D11DepthStencilView* depthStencil;
		};
		// Loading funcs
		bool LoadShaders();
		bool LoadUniforms();
		bool LoadGeometry();
		//void SetUpPipeline(PipelineHandles handles, Microsoft::WRL::ComPtr<ID3DBlob> vsBlob);
		bool SetupDrawcalls();
		Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags, std::string vertexShaderSource);
		Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags, std::string pixelShaderSource);
		PipelineHandles GetCurrentPipelineHandles();
		// Unloading funcs
		bool FreeResources();
		// Utility funcs
		std::string ShaderAsString(const char* shaderFilePath);
	private:
		// Uniform Data Definitions
		static constexpr unsigned int Instance_Max = 240;
		struct INSTANCE_UNIFORMS
		{
			GW::MATH::GMATRIXF instance_transforms[Instance_Max];
			GW::MATH::GVECTORF instance_colors[Instance_Max];
		}instanceData;
		// how many instances will be drawn this frame
		int draw_counter = 0;

		
	};
};

#endif


