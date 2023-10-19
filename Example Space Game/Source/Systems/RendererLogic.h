// The rendering system is responsible for drawing all objects
#ifndef RENDERERLOGIC_H
#define RENDERERLOGIC_H

// Contains our global game settings
#include "../GameConfig.h"


// example space game (avoid name collisions)
namespace ESG
{

	struct SceneData
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient;
		GW::MATH::GVECTORF camerPos;
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	};

	struct MeshData
	{
		GW::MATH::GMATRIXF worldMatrix[500];
		H2B::ATTRIBUTES material[300];
	};

	struct MODEL_IDS
	{
		unsigned int mod_id;
		unsigned int mat_id;
		unsigned int numLights;
		float padding;
		GW::MATH::GVECTORF color;
	};

	struct LightData
	{
		Level_Data::LIGHT_SETTINGS myLights[400];
	};


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
		GW::MATH::GMatrix proxy;
		std::shared_ptr<bool> levelChange;
		// Directx11 resources used for rendering
		std::shared_ptr<Level_Data> levelData;
		GW::GRAPHICS::GDirectX11Surface direct11;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	    indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;


		GW::MATH::GMATRIXF worldMatrix[500];
		GW::MATH::GMATRIXF viewMatrix;
		GW::MATH::GVECTORF viewTranslation;
		GW::MATH::GMATRIXF projectionMatrix;
		GW::MATH::GVECTORF lightDir;
		GW::MATH::GVECTORF lightColor;
		GW::MATH::GVECTORF lightAmbient;

		MeshData mesh;
		SceneData scene;
		MODEL_IDS modelID;
		LightData lights;
		Microsoft::WRL::ComPtr<ID3D11Buffer> constantSceneBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> constantMeshBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> constantModelBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> constantLightBuffer;

		std::string vertexShaderSource;
		std::string pixelShaderSource;
		// used to trigger clean up of vulkan resources
		GW::CORE::GEventReceiver shutdown;
	public:
		// attach the required logic to the ECS 
		bool Init(	std::shared_ptr<flecs::world> _game,
					std::weak_ptr<const GameConfig> _gameConfig,
					GW::GRAPHICS::GDirectX11Surface _direct11,
					GW::SYSTEM::GWindow _window, std::shared_ptr<Level_Data> _levelData,
			std::shared_ptr<bool> _levelChange);
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
		bool SetupDrawcalls();

		void InitializeGraphics();
		void InitializeVertexBuffer(ID3D11Device* creator);
		void InitializeIndexBuffer(ID3D11Device* creator);
		void InitializeConstantBuffer(ID3D11Device* creator);
		void InitializePipeline(ID3D11Device* creator);

		void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);
		void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);

		Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags);
		void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob);
		
		void SetUpPipeline(PipelineHandles handles);
		void ESG::D3DRendererLogic::ReleasePipelineHandles(PipelineHandles toRelease);
		PipelineHandles GetCurrentPipelineHandles();
		void LevelSwitch();
		// Unloading funcs
		bool FreeResources();
		// Utility funcs
		std::string ShaderAsString(const char* shaderFilePath);
	private:
		// Uniform Data Definitions
		/*static constexpr unsigned int Instance_Max = 240;
		struct INSTANCE_UNIFORMS
		{
			GW::MATH::GMATRIXF instance_transforms[Instance_Max];
			GW::MATH::GVECTORF instance_colors[Instance_Max];
		}instanceData;*/
		std::vector<GW::MATH::GMATRIXF> bulletMoves;
		// how many instances will be drawn this frame
		int draw_counter = 0;

		
	};
};

#endif


