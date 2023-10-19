#include "RendererLogic.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include <DDSTextureloader.h>
#include "../Components/Components.h"
#include <Shobjidl.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib") 
using namespace ESG; // Example Space Game

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

bool ESG::D3DRendererLogic::Init(	std::shared_ptr<flecs::world> _game, 
								std::weak_ptr<const GameConfig> _gameConfig,
								GW::GRAPHICS::GDirectX11Surface d3d11,
								GW::SYSTEM::GWindow _window, std::shared_ptr<const Level_Data> _levelData, std::shared_ptr<bool> _levelChange)
{
// save a handle to the ECS & game settings
game = _game;
gameConfig = _gameConfig;
direct11 = d3d11;
window = _window;
levelData = _levelData;
levelChange = _levelChange;
// Setup all vulkan resources
if (LoadShaders3D() == false)
return false;
if (LoadShaders2D() == false)
return false;
if (LoadUniforms() == false)
return false;
if (LoadGeometry() == false)
return false;
//if (SetupPipeline() == false)
//	return false;
// Setup drawing engine
if (SetupDrawcalls() == false)
return false;
// GVulkanSurface will inform us when to release any allocated resources
InitializeGraphics();

return true;
}

std::vector<Sprite>	ESG::D3DRendererLogic::LoadHudFromXML(std::string filepath)
{
	std::vector<Sprite> result;

	tinyxml2::XMLDocument document;
	tinyxml2::XMLError error_message = document.LoadFile(filepath.c_str());
	if (error_message != tinyxml2::XML_SUCCESS)
	{
		std::cout << "XML file [" + filepath + "] did not load properly." << std::endl;
		return std::vector<Sprite>();
	}

	std::string name = document.FirstChildElement("hud")->FindAttribute("name")->Value();
	GW::MATH2D::GVECTOR2F screen_size;
	screen_size.x = atof(document.FirstChildElement("hud")->FindAttribute("width")->Value());
	screen_size.y = atof(document.FirstChildElement("hud")->FindAttribute("height")->Value());

	tinyxml2::XMLElement* current = document.FirstChildElement("hud")->FirstChildElement("element");
	while (current)
	{
		Sprite s = Sprite();
		name = current->FindAttribute("name")->Value();
		FLOAT x = atof(current->FindAttribute("pos_x")->Value());
		FLOAT y = atof(current->FindAttribute("pos_y")->Value());
		FLOAT sx = atof(current->FindAttribute("scale_x")->Value());
		FLOAT sy = atof(current->FindAttribute("scale_y")->Value());
		FLOAT r = atof(current->FindAttribute("rotation")->Value());
		FLOAT d = atof(current->FindAttribute("depth")->Value());
		GW::MATH2D::GVECTOR2F s_min, s_max;
		s_min.x = atof(current->FindAttribute("sr_x")->Value());
		s_min.y = atof(current->FindAttribute("sr_y")->Value());
		s_max.x = atof(current->FindAttribute("sr_w")->Value());
		s_max.y = atof(current->FindAttribute("sr_h")->Value());
		UINT tid = atoi(current->FindAttribute("textureID")->Value());

		s.SetName(name);
		s.SetScale(sx, sy);
		s.SetPosition(x, y);
		s.SetRotation(r);
		s.SetDepth(d);
		s.SetScissorRect({ s_min, s_max });
		s.SetTextureIndex(tid);

		result.push_back(s);

		current = current->NextSiblingElement();
	}
	return result;
}

SPRITE_DATA ESG::D3DRendererLogic::UpdateSpriteConstantBufferData(const Sprite& s)
{
	SPRITE_DATA temp = { 0 };
	temp.pos_scale.x = s.GetPosition().x;
	temp.pos_scale.y = s.GetPosition().y;
	temp.pos_scale.z = s.GetScale().x;
	temp.pos_scale.w = s.GetScale().y;
	temp.rotation_depth.x = s.GetRotation();
	temp.rotation_depth.y = s.GetDepth();
	return temp;
}
SPRITE_DATA ESG::D3DRendererLogic::UpdateTextConstantBufferData(const Text& s)
{
	SPRITE_DATA temp = { 0 };
	temp.pos_scale.x = s.GetPosition().x;
	temp.pos_scale.y = s.GetPosition().y;
	temp.pos_scale.z = s.GetScale().x;
	temp.pos_scale.w = s.GetScale().y;
	temp.rotation_depth.x = s.GetRotation();
	temp.rotation_depth.y = s.GetDepth();
	return temp;
}

bool ESG::D3DRendererLogic::Activate(bool runSystem)
{
	if (startDraw.is_alive() &&
		updateDraw.is_alive() &&
		completeDraw.is_alive()) {
		if (runSystem) {
			startDraw.enable();
			updateDraw.enable();
			completeDraw.enable();
		}
		else {
			startDraw.disable();
			updateDraw.disable();
			completeDraw.disable();
		}
		return true;
	}
	return false;
}

bool ESG::D3DRendererLogic::Shutdown()
{
	startDraw.destruct();
	updateDraw.destruct();
	completeDraw.destruct();
	return true; // vulkan resource shutdown handled via GEvent in Init()
}

std::string ESG::D3DRendererLogic::ShaderAsString(const char* shaderFilePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file; file.Create();
	file.GetFileSize(shaderFilePath, stringLength);
	if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
	return output;
}

bool ESG::D3DRendererLogic::LoadShaders3D()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	vertexShader3DSource = (*readCfg).at("Shaders").at("vertex3D").as<std::string>();
	pixelShader3DSource = (*readCfg).at("Shaders").at("pixel3D").as<std::string>();
	
	if (vertexShader3DSource.empty() || pixelShader3DSource.empty())
		return false;

	vertexShader3DSource = ShaderAsString(vertexShader3DSource.c_str());
	pixelShader3DSource = ShaderAsString(pixelShader3DSource.c_str());

	if (vertexShader3DSource.empty() || pixelShader3DSource.empty())
		return false;
	
	return true;
}
bool ESG::D3DRendererLogic::LoadShaders2D()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	vertexShader2DSource = (*readCfg).at("Shaders").at("vertex2D").as<std::string>();
	pixelShader2DSource = (*readCfg).at("Shaders").at("pixel2D").as<std::string>();

	if (vertexShader2DSource.empty() || pixelShader2DSource.empty())
		return false;

	vertexShader2DSource = ShaderAsString(vertexShader2DSource.c_str());
	pixelShader2DSource = ShaderAsString(pixelShader2DSource.c_str());

	if (vertexShader2DSource.empty() || pixelShader2DSource.empty())
		return false;

	return true;
}

void ESG::D3DRendererLogic::InitializeGraphics()
{
	ID3D11Device* creator;
	direct11.GetDevice((void**)&creator);
	//InitializeVertexBuffer(creator);
	//InitializeIndexBuffer(creator);
	InitializePipeline3D(creator);
	InitializePipeline2D(creator);

	// free temporary handle
	creator->Release();
}
bool ESG::D3DRendererLogic::LoadUniforms()
{
//	VkDevice device = nullptr;
//	VkPhysicalDevice physicalDevice = nullptr;
//	vulkan.GetDevice((void**)&device);
//	vulkan.GetPhysicalDevice((void**)&physicalDevice);
//
//	unsigned max_frames = 0;
//	// to avoid per-frame resource sharing issues we give each "in-flight" frame its own buffer
//	vulkan.GetSwapchainImageCount(max_frames);
//	uniformHandle.resize(max_frames);
//	uniformData.resize(max_frames);
//	for (int i = 0; i < max_frames; ++i) {
//	
//		if (VK_SUCCESS != GvkHelper::create_buffer(physicalDevice, device,
//			sizeof(INSTANCE_UNIFORMS), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//			&uniformHandle[i], &uniformData[i]))
//			return false;
//			
//		if (VK_SUCCESS != GvkHelper::write_to_buffer( device, uniformData[i], 
//			&instanceData, sizeof(INSTANCE_UNIFORMS)))
//			return false; 
//	}
//	// uniform buffers created
	ID3D11Device* creator;
	direct11.GetDevice((void**)&creator);
	proxy.Create();

	/*viewTranslation = { 55.0f,5.0f, 25.0f, 1.0f };*/
	viewTranslation = { 140.0f, 5.0f, 0.0f, 1.0f };
	//ViewMatrix
	GW::MATH::GVECTORF viewCenter = { 0.0, 1.0f, 0.0f, 1.0f };
	GW::MATH::GVECTORF viewUp = { 0.0f, 1.0f, 0.0f, 1.0f };
	GW::MATH::GVECTORF vTranslate = { 0.0, -90.0f, 0.0f, 1.0f };
	proxy.IdentityF(viewMatrix);
	proxy.LookAtLHF(viewTranslation, viewCenter, viewUp, viewMatrix);
	proxy.TranslateLocalF(viewMatrix, vTranslate, viewMatrix);

	float ratio;
	direct11.GetAspectRatio(ratio);
	proxy.ProjectionDirectXLHF(G_DEGREE_TO_RADIAN(65.0f), ratio, 0.1f, 200.0f, projectionMatrix);

	lightDir = { -1.0f, -1.0f, -2.0f, 1.0f };
	lightColor = { 0.9f, 0.9f,1.0f, 1.0f };


	scene.viewMatrix = viewMatrix;
	scene.projectionMatrix = projectionMatrix;
	scene.sunColor = lightColor;
	scene.sunDirection = lightDir;
	

	lightAmbient = { 0.25f, 0.25f, 0.35f, 1.0f };
	scene.sunAmbient = lightAmbient;
	scene.camerPos = viewTranslation;

	return true;
}

void  ESG::D3DRendererLogic::Initialize3DVertexBuffer(ID3D11Device* creator)
{
	Create3DVertexBuffer(creator, levelData->levelVertices.data(), sizeof(H2B::VERTEX) * levelData->levelVertices.size());
}

void  ESG::D3DRendererLogic::Initialize3DIndexBuffer(ID3D11Device* creator)
{
	Create3DIndexBuffer(creator, levelData->levelIndices.data(), sizeof(unsigned int) * levelData->levelIndices.size());
}
void  ESG::D3DRendererLogic::Initialize2DVertexBuffer(ID3D11Device* creator)
{
	Create2DVertexBuffer(creator/*, levelData->levelVertices.data(), sizeof(H2B::VERTEX) * levelData->levelVertices.size()*/);
}

void  ESG::D3DRendererLogic::Initialize2DIndexBuffer(ID3D11Device* creator)
{
	Create2DIndexBuffer(creator/*, levelData->levelIndices.data(), sizeof(unsigned int) * levelData->levelIndices.size()*/);
}

void  ESG::D3DRendererLogic::InitializeConstantBuffer(ID3D11Device* creator)
{

	D3D11_SUBRESOURCE_DATA cSceneData = { &scene, 0, 0 };
	CD3D11_BUFFER_DESC cSceneDesc(sizeof(SceneData), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cSceneDesc, &cSceneData, constantSceneBuffer.ReleaseAndGetAddressOf());

	D3D11_SUBRESOURCE_DATA cMeshData = { &mesh, 0, 0 };
	CD3D11_BUFFER_DESC cMeshDesc(sizeof(MeshData), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cMeshDesc, &cMeshData, constantMeshBuffer.ReleaseAndGetAddressOf());

	D3D11_SUBRESOURCE_DATA cModelData = { &modelID, 0, 0 };
	CD3D11_BUFFER_DESC cModelDesc(sizeof(MODEL_IDS), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cModelDesc, &cModelData, constantModelBuffer.ReleaseAndGetAddressOf());

	D3D11_SUBRESOURCE_DATA cLightData = { &lights, 0, 0 };
	CD3D11_BUFFER_DESC cLightlDesc(sizeof(LightData), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cLightlDesc, &cLightData, constantLightBuffer.ReleaseAndGetAddressOf());

}

void ESG::D3DRendererLogic::Create3DVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&bDesc, &bData, vertexBuffer3D.ReleaseAndGetAddressOf());
}

void  ESG::D3DRendererLogic::Create3DIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA iData = { data, 0, 0 };
	CD3D11_BUFFER_DESC iDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&iDesc, &iData, indexBuffer3D.ReleaseAndGetAddressOf());
}

void ESG::D3DRendererLogic::Create2DVertexBuffer(ID3D11Device* creator/*, const void* data, unsigned int sizeInBytes*/)
{
	float verts[] =
	{
		-1.0f,  1.0f, 0.0f, 0.0f,		//[x,y,u,v]
		 1.0f,  1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 1.0f, 1.0f
	};

	// vertex buffer creation
	D3D11_SUBRESOURCE_DATA vbData = { verts, 0, 0 };
	CD3D11_BUFFER_DESC vbDesc(sizeof(verts), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&vbDesc, &vbData, vertexBuffer2D.GetAddressOf());
}


void ESG::D3DRendererLogic::Create2DIndexBuffer(ID3D11Device* creator/*, const void* data, unsigned int sizeInBytes*/)
{
	unsigned int indices[] =
	{
		0, 1, 2,
		1, 3, 2
	};
	// index buffer creation
	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };
	CD3D11_BUFFER_DESC ibDesc(sizeof(indices), D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&ibDesc, &ibData, indexBuffer2D.GetAddressOf());
}


void ESG::D3DRendererLogic::InitializePipeline3D(ID3D11Device* creator)
{
	//Initialixe pipeline
	direct11.GetDevice((void**)&creator);
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader3D(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader3D(creator, compilerFlags);
	Create3DVertexInputLayout(creator, vsBlob);
}
void ESG::D3DRendererLogic::InitializePipeline2D(ID3D11Device* creator)
{
	//Initialixe pipeline
	direct11.GetDevice((void**)&creator);
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader2D(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader2D(creator, compilerFlags);
	Create2DVertexInputLayout(creator, vsBlob);
}

Microsoft::WRL::ComPtr<ID3DBlob> ESG::D3DRendererLogic::CompilePixelShader3D(ID3D11Device* creator, UINT compilerFlags)
{

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	HRESULT compilationResult =
		D3DCompile(pixelShader3DSource.c_str(), pixelShader3DSource.length(),
			nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(), nullptr, pixelShader3D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob>  ESG::D3DRendererLogic::CompileVertexShader3D(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	HRESULT compilationResult =
		D3DCompile(vertexShader3DSource.c_str(), vertexShader3DSource.length(),
			nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), nullptr, vertexShader3D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

Microsoft::WRL::ComPtr<ID3DBlob> ESG::D3DRendererLogic::CompilePixelShader2D(ID3D11Device* creator, UINT compilerFlags)
{

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	HRESULT compilationResult =
		D3DCompile(pixelShader2DSource.c_str(), pixelShader2DSource.length(),
			nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(), nullptr, pixelShader2D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob>  ESG::D3DRendererLogic::CompileVertexShader2D(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	HRESULT compilationResult =
		D3DCompile(vertexShader2DSource.c_str(), vertexShader2DSource.length(),
			nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), nullptr, vertexShader2D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

void ESG::D3DRendererLogic::Create3DVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
{
	D3D11_INPUT_ELEMENT_DESC attributes[3];

	attributes[0].SemanticName = "POSITION";
	attributes[0].SemanticIndex = 0;
	attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[0].InputSlot = 0;
	attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[0].InstanceDataStepRate = 0;

	attributes[1].SemanticName = "UV";
	attributes[1].SemanticIndex = 0;
	attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[1].InputSlot = 0;
	attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[1].InstanceDataStepRate = 0;

	attributes[2].SemanticName = "NORMAL";
	attributes[2].SemanticIndex = 0;
	attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[2].InputSlot = 0;
	attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[2].InstanceDataStepRate = 0;

	creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		vertexFormat3D.GetAddressOf());
}
void ESG::D3DRendererLogic::Create2DVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
{
	D3D11_INPUT_ELEMENT_DESC format[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	creator->CreateInputLayout(format, ARRAYSIZE(format),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		vertexFormat2D.GetAddressOf());
}
bool ESG::D3DRendererLogic::LoadGeometry()
{
	ID3D11Device* creator;
	direct11.GetDevice((void**)&creator);
	proxy.Create();
	inputProxy.Create(window);

	/*viewTranslation = { 55.0f,5.0f, 25.0f, 1.0f };*/
	viewTranslation = { 140.0f, 5.0f, 0.0f, 1.0f };
	//ViewMatrix
	GW::MATH::GVECTORF viewCenter = { 0.0, 1.0f, 0.0f, 1.0f };
	GW::MATH::GVECTORF viewUp = { 0.0f, 1.0f, 0.0f, 1.0f };
	GW::MATH::GVECTORF vTranslate = { 0.0, -90.0f, 0.0f, 1.0f };
	proxy.IdentityF(viewMatrix);
	proxy.LookAtLHF(viewTranslation, viewCenter, viewUp, viewMatrix);
	proxy.TranslateLocalF(viewMatrix, vTranslate, viewMatrix);

	float ratio;
	direct11.GetAspectRatio(ratio);
	proxy.ProjectionDirectXLHF(G_DEGREE_TO_RADIAN(65.0f), ratio, 0.1f, 200.0f, projectionMatrix);

	lightDir = { -1.0f, -1.0f, -2.0f, 1.0f };
	lightColor = { 0.9f, 0.9f,1.0f, 1.0f };


	scene.viewMatrix = viewMatrix;
	scene.projectionMatrix = projectionMatrix;
	scene.sunColor = lightColor;
	scene.sunDirection = lightDir;
	modelID.mat_id = levelData->levelMeshes[0].materialIndex;
	modelID.mod_id = levelData->levelInstances[0].modelIndex;
	modelID.numLights = levelData->levelLighting.size();

	lightAmbient = { 0.25f, 0.25f, 0.35f, 1.0f };
	scene.sunAmbient = lightAmbient;
	scene.camerPos = viewTranslation;

	Initialize3DVertexBuffer(creator);
	Initialize3DIndexBuffer(creator);
	InitializeConstantBuffer(creator);
	for (int i = 0; i < levelData->levelMaterials.size(); ++i)
	{
		mesh.material[i] = levelData->levelMaterials[i].attrib;
	}

	for (int i = 0; i < levelData->levelTransforms.size(); ++i)
	{
		mesh.worldMatrix[i] = levelData->levelTransforms[i];
	}
	modelID.numLights = levelData->levelLighting.size();
	modelID.mat_id = levelData->levelMeshes[0].materialIndex;
	modelID.mod_id = levelData->levelInstances[0].modelIndex;
	for (int i = 0; i < levelData->levelLighting.size(); ++i)
	{
		lights.myLights[i] = levelData->levelLighting[i];
	}
	//std::vector<float> verts = {
	//	-0.5f, -0.5f,
	//	0, 0.5f,
	//	0, -0.25f,
	//	0.5f, -0.5f
	//};
	// Transfer triangle data to the vertex buffer. (staging buffer would be prefered here)
	Initialize3DVertexBuffer(creator);
	Initialize3DIndexBuffer(creator);


	unsigned int width;
	unsigned int height;

	CD3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	creator->CreateBlendState(&blendDesc, blendState.GetAddressOf());

	// creation of the depth stencil state
	// this is used to blend with objects when they are on the same z-plane
	// the depth function needs to be set to less_equal instead of less
	CD3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	creator->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());

	CD3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.ScissorEnable = true;
	creator->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());

	// when loading a texture from disk to an interface object for directx 11
	// we use this function call "CreateDDSTextureFromFile"
	// this requires a wide string as a parameter for the file path
	// the shaderResourceView is a directx 11 interface object that points to the texture information on the gpu

	// an array to store all of the texture names
	// this makes looping over and creating shader resource views easier
	std::wstring texture_names[] =
	{
		L"greendragon.dds",
		L"HUD_Sharp_backplate.dds",
		L"Health_left.dds",
		L"Health_right.dds",
		L"Mana_left.dds",
		L"Mana_right.dds",
		L"Stamina_backplate.dds",
		L"Stamina.dds",
		L"Center_top.dds",
		L"font_consolas_32.dds"
	};

	// loop used for creating shader resource views
	for (size_t i = 0; i < ARRAYSIZE(texture_names); i++)
	{
		// create a wide string to store the file path and file name
		std::wstring texturePath = LTEXTURES_PATH;
		texturePath += texture_names[i];
		// load texture from disk 
		DirectX::CreateDDSTextureFromFile(creator, texturePath.c_str(), nullptr, shaderResourceView[i].GetAddressOf());
	}

	// samplerStates are needed when using textures
	// this is for filtering the texture
	// some options are bilinear, trilinear, anisotropic, etc..
	CD3D11_SAMPLER_DESC samp_desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	creator->CreateSamplerState(&samp_desc, samplerState.GetAddressOf());

	// this is where we create the constant buffer
	// it is used to store constant data for each draw call
	// we will use this to send sprite data to the vertex shader
	D3D11_SUBRESOURCE_DATA cbData = { &constantBufferData, 0, 0 };
	// DEFAULT usage lets us use UpdateSubResource
	// DYNAMIC usage lets us use Map / Unmap
	CD3D11_BUFFER_DESC cbDesc(sizeof(constantBufferData), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cbDesc, &cbData, constantBufferHUD.GetAddressOf());

	// store the current width and height of the client's window
	window.GetClientWidth(width);
	window.GetClientHeight(height);
	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = XML_PATH;
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	staticTextHS = Text();
	staticTextHS.SetText("HIGHSCORE:");
	staticTextHS.SetFont(&consolas32);
	staticTextHS.SetPosition(0.67f, 0.7f);
	staticTextHS.SetScale(0.75f, 0.5f);
	staticTextHS.SetRotation(0.0f);
	staticTextHS.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticTextHS.Update(width, height);

	// vertex buffer creation for the staticText
	const auto& staticVerts = staticTextHS.GetVertices();
	D3D11_SUBRESOURCE_DATA svbData = { staticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC svbDesc(sizeof(TextVertex) * staticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&svbDesc, &svbData, vertexBufferStaticTextHS.GetAddressOf());

	dynamicTextHS = Text();
	dynamicTextHS.SetFont(&consolas32);
	dynamicTextHS.SetPosition(0.67f, 0.65f);
	dynamicTextHS.SetScale(0.75f, 0.5f);
	dynamicTextHS.SetRotation(0.0f);
	dynamicTextHS.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	dynamicTextHS.Update(width, height);

	// vertex buffer creation for the staticText
	CD3D11_BUFFER_DESC dvbDesc(sizeof(TextVertex) * 6 * 5000, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	creator->CreateBuffer(&dvbDesc, nullptr, vertexBufferDynamicTextHS.GetAddressOf());

	staticTextTime = Text();
	staticTextTime.SetText("TIME:");
	staticTextTime.SetFont(&consolas32);
	staticTextTime.SetPosition(0.67f, 0.85f);
	staticTextTime.SetScale(0.75f, 0.5f);
	staticTextTime.SetRotation(0.0f);
	staticTextTime.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticTextTime.Update(width, height);

	// vertex buffer creation for the staticText
	const auto& TstaticVerts = staticTextTime.GetVertices();
	D3D11_SUBRESOURCE_DATA tsvbData = { TstaticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC tsvbDesc(sizeof(TextVertex) * TstaticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&tsvbDesc, &tsvbData, vertexBufferStaticTextTime.GetAddressOf());

	dynamicTextTime = Text();
	dynamicTextTime.SetFont(&consolas32);
	dynamicTextTime.SetPosition(0.67f, 0.8f);
	dynamicTextTime.SetScale(0.75f, 0.5f);
	dynamicTextTime.SetRotation(0.0f);
	dynamicTextTime.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	dynamicTextTime.Update(width, height);

	// vertex buffer creation for the staticText
	CD3D11_BUFFER_DESC tdvbDesc(sizeof(TextVertex) * 6 * 5000, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	creator->CreateBuffer(&tdvbDesc, nullptr, vertexBufferDynamicTextTime.GetAddressOf());

	staticTextLives = Text();
	staticTextLives.SetText("LIVES:");
	staticTextLives.SetFont(&consolas32);
	staticTextLives.SetPosition(-0.6f, -0.85f);
	staticTextLives.SetScale(1.25f, 1.25f);
	staticTextLives.SetRotation(0.0f);
	staticTextLives.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticTextLives.Update(width, height);

	// vertex buffer creation for the staticText
	const auto& LstaticVerts = staticTextLives.GetVertices();
	D3D11_SUBRESOURCE_DATA lsvbData = { LstaticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC lsvbDesc(sizeof(TextVertex) * LstaticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&lsvbDesc, &lsvbData, vertexBufferStaticTextLives.GetAddressOf());

	staticTextWin = Text();
	staticTextWin.SetText("YOU WIN");
	staticTextWin.SetFont(&consolas32);
	staticTextWin.SetPosition(0.0f, 0.0f);
	staticTextWin.SetScale(2.0f, 2.0f);
	staticTextWin.SetRotation(0.0f);
	staticTextWin.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticTextWin.Update(width, height);

	// vertex buffer creation for the staticText
	const auto& WstaticVerts = staticTextWin.GetVertices();
	D3D11_SUBRESOURCE_DATA wsvbData = { WstaticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC wsvbDesc(sizeof(TextVertex)* WstaticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&wsvbDesc, &wsvbData, vertexBufferStaticTextWin.GetAddressOf());

	staticTextLose = Text();
	staticTextLose.SetText("YOU LOSE");
	staticTextLose.SetFont(&consolas32);
	staticTextLose.SetPosition(0.0f, 0.0f);
	staticTextLose.SetScale(2.0f, 2.0f);
	staticTextLose.SetRotation(0.0f);
	staticTextLose.SetDepth(0.0f);
	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticTextLose.Update(width, height);

	// vertex buffer creation for the staticText
	const auto& LSstaticVerts = staticTextLose.GetVertices();
	D3D11_SUBRESOURCE_DATA lssvbData = { LSstaticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC lssvbDesc(sizeof(TextVertex)* LSstaticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&lssvbDesc, &lssvbData, vertexBufferStaticTextLose.GetAddressOf());

	creator->Release();
	return true;
}

ESG::D3DRendererLogic::PipelineHandles ESG::D3DRendererLogic::GetCurrentPipelineHandles()
{
	PipelineHandles retval;
	direct11.GetImmediateContext((void**)&retval.context);
	direct11.GetRenderTargetView((void**)&retval.targetView);
	direct11.GetDepthStencilView((void**)&retval.depthStencil);
	return retval;
}

void ESG::D3DRendererLogic::SetUpPipeline(PipelineHandles handles)
{
	//float blendFactor[] = {0.799f, 0.799f, 0.799f, 1.0f};
	//Set Render Targets
	ID3D11RenderTargetView* const views[] = { handles.targetView };
	handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	
	handles.context->OMSetBlendState(blendState.Get(), NULL, 0xFFFFFFFF);
	// set the depth stencil state for depth comparison [useful for transparency with the hud objects]
	handles.context->OMSetDepthStencilState(depthStencilState.Get(), 0xFFFFFFFF);
	// set the rasterization state for use with the scissor rectangle
	handles.context->RSSetState(rasterizerState.Get());

	//Set Vertex Buffers
	const UINT strides[] = { sizeof(H2B::VERTEX) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer3D.Get() };
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(indexBuffer3D.Get(), DXGI_FORMAT_R32_UINT, 0);

	//Set Shaders
	handles.context->VSSetShader(vertexShader3D.Get(), nullptr, 0);
	handles.context->PSSetShader(pixelShader3D.Get(), nullptr, 0);

	//// Create Stage Info for Vertex Shader
	//

	//// Create Stage Info for Fragment Shader

	handles.context->VSSetConstantBuffers(1, 1, constantMeshBuffer.GetAddressOf());
	handles.context->VSSetConstantBuffers(0, 1, constantSceneBuffer.GetAddressOf());
	handles.context->VSSetConstantBuffers(2, 1, constantModelBuffer.GetAddressOf());
	handles.context->VSSetConstantBuffers(3, 1, constantLightBuffer.GetAddressOf());


	handles.context->PSSetConstantBuffers(1, 1, constantMeshBuffer.GetAddressOf());
	handles.context->PSSetConstantBuffers(0, 1, constantSceneBuffer.GetAddressOf());
	handles.context->PSSetConstantBuffers(2, 1, constantModelBuffer.GetAddressOf());
	handles.context->PSSetConstantBuffers(3, 1, constantLightBuffer.GetAddressOf());
	// Assembly State
	handles.context->IASetInputLayout(vertexFormat3D.Get());
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Vertex Input State

	//// viewport state (we still need to set this up even though we will overwrite the values)
	//

	//// Rasterizer State
	//

	//// Multisampling State
	//

	//// Depth-Stencil State
	
}

void ESG::D3DRendererLogic::ReleasePipelineHandles(PipelineHandles toRelease)
{
	toRelease.depthStencil->Release();
	toRelease.targetView->Release();
	toRelease.context->Release();
}

bool ESG::D3DRendererLogic::SetupDrawcalls() // I SCREWED THIS UP MAKES SO MUCH SENSE NOW
{
	// create a unique entity for the renderer (just a Tag)
	// this only exists to ensure we can create systems that will run only once per frame. 
	game->entity("Rendering System").add<RenderingSystem>();
	// an instanced renderer is complex and needs to run additional system code once per frame
	// to do this I create 3 systems:
	// A pre-update system, that runs only once (using our Tag above)
	// An update system that iterates over all renderable components (may run multiple times)
	// A post-update system that also runs only once rendering all collected data

	// only happens once per frame
	startDraw = game->system<RenderingSystem>().kind(flecs::PreUpdate)
		.each([this](flecs::entity e, RenderingSystem& s) {
		// reset the draw counter only once per frame
		draw_counter = 0;
		//loop over levelData->levelTransforms
		//copy over to mesh.WorldMatrix[i] = levelTransforms
		for (int i = 0; i < levelData->levelTransforms.size(); ++i)
		{
			mesh.worldMatrix[i] = levelData->levelTransforms[i];
		}
			});
	// may run multiple times per frame, will run after startDraw
	updateDraw = game->system<Position, Orientation, Material>().kind(flecs::OnUpdate)
		.each([this](flecs::entity e, Position& p, Orientation& o, Material& m) {
		// copy all data to our instancing array
		if (e.has<BulletTest>())
		{
			GW::MATH::GMATRIXF bullet = GW::MATH::GIdentityMatrixF;
			bullet.row4.x = p.value.x;
			bullet.row4.y = p.value.y;

			bullet.row1.x = o.value.row1.x;
			bullet.row1.y = o.value.row1.y;
			bullet.row2.x = o.value.row2.x;
			bullet.row2.y = o.value.row2.y;
			bulletMoves.push_back(bullet);
			LevelSwitch();
			//int i = draw_counter;
			//instanceData.instance_transforms[i] = GW::MATH::GIdentityMatrixF;
			//instanceData.instance_transforms[i].row4.x = p.value.x;
			//instanceData.instance_transforms[i].row4.y = p.value.y;
			//// transfer 2D orientation to 4x4
			//instanceData.instance_transforms[i].row1.x = o.value.row1.x;
			//instanceData.instance_transforms[i].row1.y = o.value.row1.y;
			//instanceData.instance_transforms[i].row2.x = o.value.row2.x;
			//instanceData.instance_transforms[i].row2.y = o.value.row2.y;
			//// set color
			//instanceData.instance_colors[i].x = m.diffuse.value.x;
			//instanceData.instance_colors[i].y = m.diffuse.value.y;
			//instanceData.instance_colors[i].z = m.diffuse.value.z;
			//instanceData.instance_colors[i].w = 1; // opaque
			//// increment the shared draw counter but don't go over (branchless) 
			//int v = static_cast<int>(Instance_Max) - static_cast<int>(draw_counter + 2);
			//// if v < 0 then 0, else 1, https://graphics.stanford.edu/~seander/bithacks.html
			//int sign = 1 ^ ((unsigned int)v >> (sizeof(int) * CHAR_BIT - 1));
			//draw_counter += sign;
		}
			});

	// runs once per frame after updateDraw
	completeDraw = game->system<Instance>().kind(flecs::PostUpdate)
		.each([this](flecs::entity e, Instance& s) {
		// run the rendering code just once!
		// Copy data to this frame's buffer
		//VkDevice device = nullptr;
		//vulkan.GetDevice((void**)&device);
		//unsigned int activeBuffer;
		//vulkan.GetSwapchainCurrentImage(activeBuffer);
		//GvkHelper::write_to_buffer(device, 
		//	uniformData[activeBuffer], &instanceData, sizeof(INSTANCE_UNIFORMS));
		//// grab the current Vulkan commandBuffer
		//unsigned int currentBuffer;
		//vulkan.GetSwapchainCurrentImage(currentBuffer);
		//VkCommandBuffer commandBuffer;
		//vulkan.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		//// what is the current client area dimensions?
		//unsigned int width, height;
		//window.GetClientWidth(width);
		//window.GetClientHeight(height);
		//// setup the pipeline's dynamic settings
		//VkViewport viewport = {
		//	0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		//};
		//VkRect2D scissor = { {0, 0}, {width, height} };
		//vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		//vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		//// Set the descriptorSet that contains the uniform buffer allocated for this framebuffer 
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		//	pipelineLayout, 0, 1, &descriptorSet[currentBuffer], 0, nullptr);
		//// now we can draw
		//VkDeviceSize offsets[] = { 0 };
		//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		//vkCmdDraw(commandBuffer, 4, draw_counter, 0, 0); // draw'em all!
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);
		curHandles.context->UpdateSubresource(constantSceneBuffer.Get(), 0, nullptr, &scene, 0, 0);
		curHandles.context->UpdateSubresource(constantMeshBuffer.Get(), 0, nullptr, &mesh, 0, 0);
		curHandles.context->UpdateSubresource(constantLightBuffer.Get(), 0, nullptr, &lights, 0, 0);

		modelID.mod_id = e.get<Instance>()->transformStart;
		/*if (e.has<BulletTest>())
		{
			for (int i = 0; i < (int)bulletMoves.size(); i++)
			{
				curHandles.context->UpdateSubresource(constantMeshBuffer.Get(), 0, nullptr, &mesh, 0, 0);
				auto meshCount = e.get_mut<Object>()->meshStart;
				modelID.mat_id = levelData->levelMeshes[meshCount].materialIndex + e.get_mut<Object>()->materialStart;

				auto colorModel = e.get_mut<Material>()->diffuse.value;
				modelID.color = GW::MATH::GVECTORF{ colorModel.x, colorModel.y, colorModel.z, 1 };
				curHandles.context->UpdateSubresource(constantModelBuffer.Get(), 0, nullptr, &modelID, 0, 0);
				curHandles.context->DrawIndexedInstanced(levelData->levelMeshes[2].drawInfo.indexCount,
					draw_counter,
					levelData->levelMeshes[2].drawInfo.indexOffset + e.get_mut<Object>()->indexStart
					,e.get_mut<Object>()->vertexStart, 0);
			}
		}
		else {*/
		for (unsigned int j = 0; j < e.get<Object>()->meshCount; ++j)
		{
			auto meshCount = e.get<Object>()->meshStart + j;
			modelID.mat_id = levelData->levelMeshes[meshCount].materialIndex + e.get<Object>()->materialStart;

			auto colorModel = e.get<Material>()->diffuse.value;
			modelID.color = GW::MATH::GVECTORF{ colorModel.x, colorModel.y, colorModel.z, 1 };

			curHandles.context->UpdateSubresource(constantModelBuffer.Get(), 0, nullptr, &modelID, 0, 0);

			curHandles.context->DrawIndexedInstanced(levelData->levelMeshes[meshCount].drawInfo.indexCount,
				e.get<Instance>()->transformCount,
				levelData->levelMeshes[meshCount].drawInfo.indexOffset + e.get<Object>()->indexStart,
				e.get<Object>()->vertexStart,
				0);

		}
		/*}*/




		ReleasePipelineHandles(curHandles);
		UIDraw();
			});
	// NOTE: I went with multi-system approach for the ease of passing lambdas with "this"
	// There is a built-in solution for this problem referred to as a "custom runner":
	// https://github.com/SanderMertens/flecs/blob/master/examples/cpp/systems/custom_runner/src/main.cpp
	// The negative is that it requires the use of a C callback which is less flexibe vs the lambda
	// you could embed what you need in the ecs and use a lookup to get it but I think that is less clean

	// all drawing operations have been setup
	return true;
}

void ESG::D3DRendererLogic::UIDraw()
{
	PipelineHandles curHandles = GetCurrentPipelineHandles();
	SetUpPipeline(curHandles);
	curHandles.context->VSSetShader(vertexShader2D.Get(), nullptr, 0);
	curHandles.context->PSSetShader(pixelShader2D.Get(), nullptr, 0);

	const UINT strides[] = { sizeof(float) * 4 };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer2D.Get() };
	// set the vertex buffer to the pipeline
	curHandles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	curHandles.context->IASetIndexBuffer(indexBuffer2D.Get(), DXGI_FORMAT_R32_UINT, 0);
	curHandles.context->VSSetShader(vertexShader2D.Get(), nullptr, 0);
	curHandles.context->PSSetShader(pixelShader2D.Get(), nullptr, 0);
	curHandles.context->IASetInputLayout(vertexFormat2D.Get());
	// set the topology
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	// set and update the constant buffer (cb)
	curHandles.context->VSSetConstantBuffers(0, 1, constantBufferHUD.GetAddressOf());

	curHandles.context->IASetVertexBuffers(0, 1, vertexBufferStaticTextTime.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferData = UpdateTextConstantBufferData(staticTextTime);
	// bind the texture used for rendering the font
	curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
	// draw the static text using the number of vertices
	curHandles.context->Draw(staticTextTime.GetVertices().size(), 0);

	unsigned int width;
	unsigned int height;
	window.GetWidth(width);
	window.GetHeight(height);
	static auto start = std::chrono::steady_clock::now();
	double elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double>(std::chrono::steady_clock::now() - start)).count();
	double elapsedMin = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::duration<double>(std::chrono::steady_clock::now() - start)).count();
	if (int(elapsedSec) >= 60)
	{
		elapsedSec -= 60 * int(elapsedSec/60);
	}
	dynamicTextTime.SetText("0" + std::to_string(int(elapsedMin)) + ":" + "0" + std::to_string(int(elapsedSec)));
	if (elapsedSec >= 10 || elapsedMin >= 10)
	{
		if (elapsedSec >= 10 || elapsedMin >= 10)
		{
			dynamicTextTime.SetText("0" + std::to_string(int(elapsedMin)) + ":" + std::to_string(int(elapsedSec)));
		}
		if (elapsedMin >= 10)
		{
			dynamicTextTime.SetText(std::to_string(int(elapsedMin)) + ":0" + std::to_string(int(elapsedSec)));
		}
	}
	// update the dynamic text so we create the vertices
	dynamicTextTime.Update(width, height);
	// upload the new information to the vertex buffer using map / unmap
	const auto& verts = dynamicTextTime.GetVertices();
	D3D11_MAPPED_SUBRESOURCE msr = { 0 };
	curHandles.context->Map(vertexBufferDynamicTextTime.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, verts.data(), sizeof(TextVertex) * verts.size());
	curHandles.context->Unmap(vertexBufferDynamicTextTime.Get(), 0);
	
	curHandles.context->IASetVertexBuffers(0, 1, vertexBufferDynamicTextTime.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferData = UpdateTextConstantBufferData(dynamicTextTime);
	// bind the texture used for rendering the font
	curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
	// draw the static text using the number of vertices
	curHandles.context->Draw(dynamicTextTime.GetVertices().size(), 0);

	curHandles.context->IASetVertexBuffers(0, 1, vertexBufferStaticTextHS.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferData = UpdateTextConstantBufferData(staticTextHS);
	// bind the texture used for rendering the font
	curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
	// draw the static text using the number of vertices
	curHandles.context->Draw(staticTextHS.GetVertices().size(), 0);

	dynamicTextHS.SetText(std::to_string(elapsedSec));
	// update the dynamic text so we create the vertices
	dynamicTextHS.Update(width, height);
	const auto& HSverts = dynamicTextHS.GetVertices();
	D3D11_MAPPED_SUBRESOURCE HSmsr = { 0 };
	curHandles.context->Map(vertexBufferDynamicTextHS.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &HSmsr);
	memcpy(HSmsr.pData, HSverts.data(), sizeof(TextVertex) * HSverts.size());
	curHandles.context->Unmap(vertexBufferDynamicTextHS.Get(), 0);

	curHandles.context->IASetVertexBuffers(0, 1, vertexBufferDynamicTextHS.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferData = UpdateTextConstantBufferData(dynamicTextHS);
	// bind the texture used for rendering the font
	curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
	// draw the static text using the number of vertices
	curHandles.context->Draw(dynamicTextHS.GetVertices().size(), 0);

	curHandles.context->IASetVertexBuffers(0, 1, vertexBufferStaticTextLives.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferData = UpdateTextConstantBufferData(staticTextLives);
	// bind the texture used for rendering the font
	curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
	// draw the static text using the number of vertices
	curHandles.context->Draw(staticTextLives.GetVertices().size(), 0);
	float one;
	float two;
	inputProxy.GetState(65, one);
	inputProxy.GetState(66, two);

	if (one > 0 || conditionWin)
	{
		conditionLose = false;
		curHandles.context->IASetVertexBuffers(0, 1, vertexBufferStaticTextWin.GetAddressOf(), strides, offsets);
		// change the topology to a triangle list
		curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// update the constant buffer data for the text
		constantBufferData = UpdateTextConstantBufferData(staticTextWin);
		// bind the texture used for rendering the font
		curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
		// update the constant buffer with the text's data
		curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
		// draw the static text using the number of vertices
		curHandles.context->Draw(staticTextWin.GetVertices().size(), 0);
		conditionWin = true;
	}
	if (two > 0 || conditionLose)
	{
		conditionWin = false;
		curHandles.context->IASetVertexBuffers(0, 1, vertexBufferStaticTextLose.GetAddressOf(), strides, offsets);
		// change the topology to a triangle list
		curHandles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// update the constant buffer data for the text
		constantBufferData = UpdateTextConstantBufferData(staticTextLose);
		// bind the texture used for rendering the font
		curHandles.context->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
		// update the constant buffer with the text's data
		curHandles.context->UpdateSubresource(constantBufferHUD.Get(), 0, nullptr, &constantBufferData, 0, 0);
		// draw the static text using the number of vertices
		curHandles.context->Draw(staticTextLose.GetVertices().size(), 0);
		conditionLose = true;
	}

}

bool ESG::D3DRendererLogic::FreeResources()
{
	//VkDevice device = nullptr;
	//vulkan.GetDevice((void**)&device);
	//// wait till everything has completed
	//vkDeviceWaitIdle(device);
	//// free the uniform buffer and its handle
	//for (int i = 0; i < uniformData.size(); ++i) {
	//	vkDestroyBuffer(device, uniformHandle[i], nullptr);
	//	vkFreeMemory(device, uniformData[i], nullptr);
	//}
	//uniformData.clear(); 
	//uniformHandle.clear();
	//// don't need the descriptors anymore
	//vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
	//vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	//// Release allocated buffers, shaders & pipeline
	//vkDestroyBuffer(device, vertexHandle, nullptr);
	//vkFreeMemory(device, vertexData, nullptr);
	//vkDestroyShaderModule(device, vertexShader, nullptr);
	//vkDestroyShaderModule(device, pixelShader, nullptr);
	//vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	//vkDestroyPipeline(device, pipeline, nullptr);

	return true;
}
void ESG::D3DRendererLogic::LevelSwitch()
{
	if (*levelChange)
	{
		IShellItem* pShellItem = nullptr;
		COMDLG_FILTERSPEC ComDlgFS[1] = { {L"Text Files", L"*.txt"} };
		//LPWSTR filePath;
		HRESULT he = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(he))
		{
			IFileDialog* pFileOpen = nullptr;
			he = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)(&pFileOpen));
			if (SUCCEEDED(he))
			{
				pFileOpen->SetFileTypes(1, ComDlgFS);
				pFileOpen->SetTitle(L"Level Selection");
				he = pFileOpen->Show(0);
				if (SUCCEEDED(he))
				{
					wchar_t* filePath;
					he = pFileOpen->GetResult(&pShellItem);
					if (SUCCEEDED(he))
					{
						pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
						std::wstring file;
						if (filePath != 0)
						{
							file = std::wstring(filePath);
							std::string fileName(file.begin(), file.end());
							std::string base_file = fileName.substr(fileName.find_last_of("/\\") + 1);
							std::string search = "../" + base_file;
							GW::SYSTEM::GLog log;
							
							bool levelLoaded = levelData->LoadLevel(search.c_str(), "../Models", log);
							if (LoadGeometry())
							{
								PipelineHandles handles = GetCurrentPipelineHandles();
								SetUpPipeline(handles);

							}
							(*levelChange) = false;
							CoTaskMemFree(filePath);
							pShellItem->Release();
						}

					}
				}
				pFileOpen->Release();
			}
			CoUninitialize();
		}
	}
}