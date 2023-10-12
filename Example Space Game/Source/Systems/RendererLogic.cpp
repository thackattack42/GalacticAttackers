#include "RendererLogic.h"
#include "../Components/Identification.h"
#include "../Components/Visuals.h"
#include "../Components/Physics.h"
#include "../Components/Components.h"
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
								GW::SYSTEM::GWindow _window, std::shared_ptr<const Level_Data> _levelData)
{
	// save a handle to the ECS & game settings
	game = _game;
	gameConfig = _gameConfig;
	direct11 = d3d11;
	window = _window;
	levelData = _levelData;

	
	// Setup all vulkan resources
	if (LoadShaders() == false) 
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

bool ESG::D3DRendererLogic::LoadShaders()
{
	std::shared_ptr<const GameConfig> readCfg = gameConfig.lock();
	vertexShaderSource = (*readCfg).at("Shaders").at("vertex").as<std::string>();
	pixelShaderSource = (*readCfg).at("Shaders").at("pixel").as<std::string>();
	
	if (vertexShaderSource.empty() || pixelShaderSource.empty())
		return false;

	vertexShaderSource = ShaderAsString(vertexShaderSource.c_str());
	pixelShaderSource = ShaderAsString(pixelShaderSource.c_str());

	if (vertexShaderSource.empty() || pixelShaderSource.empty())
		return false;
	

}

void ESG::D3DRendererLogic::InitializeGraphics()
{
	ID3D11Device* creator;
	direct11.GetDevice((void**)&creator);
	//InitializeVertexBuffer(creator);
	//InitializeIndexBuffer(creator);
	InitializePipeline(creator);

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
	InitializeConstantBuffer(creator);
	return true;
}

void  ESG::D3DRendererLogic::InitializeVertexBuffer(ID3D11Device* creator)
{
	CreateVertexBuffer(creator, levelData->levelVertices.data(), sizeof(H2B::VERTEX) * levelData->levelVertices.size());
}

void  ESG::D3DRendererLogic::InitializeIndexBuffer(ID3D11Device* creator)
{
	CreateIndexBuffer(creator, levelData->levelIndices.data(), sizeof(unsigned int) * levelData->levelIndices.size());
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

void ESG::D3DRendererLogic::CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&bDesc, &bData, vertexBuffer.ReleaseAndGetAddressOf());
}

void  ESG::D3DRendererLogic::CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA iData = { data, 0, 0 };
	CD3D11_BUFFER_DESC iDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&iDesc, &iData, indexBuffer.ReleaseAndGetAddressOf());
}


void ESG::D3DRendererLogic::InitializePipeline(ID3D11Device* creator)
{
	//Initialixe pipeline
	direct11.GetDevice((void**)&creator);
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);
	CreateVertexInputLayout(creator, vsBlob);
}

Microsoft::WRL::ComPtr<ID3DBlob> ESG::D3DRendererLogic::CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
{

	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	HRESULT compilationResult =
		D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
			nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob>  ESG::D3DRendererLogic::CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	HRESULT compilationResult =
		D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
			nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}

void ESG::D3DRendererLogic::CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
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
		vertexFormat.GetAddressOf());
}

bool ESG::D3DRendererLogic::LoadGeometry()
{
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
	modelID.mat_id = levelData->levelMeshes[0].materialIndex;
	modelID.mod_id = levelData->levelInstances[0].modelIndex;
	modelID.numLights = levelData->levelLighting.size();

	lightAmbient = { 0.25f, 0.25f, 0.35f, 1.0f };
	scene.sunAmbient = lightAmbient;
	scene.camerPos = viewTranslation;

	for (int i = 0; i < levelData->levelMaterials.size(); ++i)
	{
		mesh.material[i] = levelData->levelMaterials[i].attrib;
	}

	for (int i = 0; i < levelData->levelTransforms.size(); ++i)
	{
		mesh.worldMatrix[i] = levelData->levelTransforms[i];
	}

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
	InitializeVertexBuffer(creator);
	InitializeIndexBuffer(creator);
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
	//Set Render Targets
	ID3D11RenderTargetView* const views[] = { handles.targetView };
	handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	
	//Set Vertex Buffers
	const UINT strides[] = { sizeof(H2B::VERTEX) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	//Set Shaders
	handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
	handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);

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
	handles.context->IASetInputLayout(vertexFormat.Get());
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// Vertex Input State

	//// viewport state (we still need to set this up even though we will overwrite the values)
	//

	//// Rasterizer State
	//

	//// Multisampling State
	//

	//// Depth-Stencil State
	//VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
	//depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	//depth_stencil_create_info.depthTestEnable = VK_TRUE;
	//depth_stencil_create_info.depthWriteEnable = VK_TRUE;
	//depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
	//depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
	//depth_stencil_create_info.minDepthBounds = 0.0f;
	//depth_stencil_create_info.maxDepthBounds = 1.0f;
	//depth_stencil_create_info.stencilTestEnable = VK_FALSE;
	//// Color Blending Attachment & State
	//VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
	//color_blend_attachment_state.colorWriteMask = 0xF;
	//color_blend_attachment_state.blendEnable = VK_FALSE;
	//color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	//color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
	//color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	//color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	//color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
	//VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
	//color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	//color_blend_create_info.logicOpEnable = VK_FALSE;
	//color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
	//color_blend_create_info.attachmentCount = 1;
	//color_blend_create_info.pAttachments = &color_blend_attachment_state;
	//color_blend_create_info.blendConstants[0] = 0.0f;
	//color_blend_create_info.blendConstants[1] = 0.0f;
	//color_blend_create_info.blendConstants[2] = 0.0f;
	//color_blend_create_info.blendConstants[3] = 0.0f;
	//// Dynamic State 
	//VkDynamicState dynamic_state[2] = { 
	//	// By setting these we do not need to re-create the pipeline on Resize
	//	VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	//};
	//VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
	//dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamic_create_info.dynamicStateCount = 2;
	//dynamic_create_info.pDynamicStates = dynamic_state;
	//// Descriptor Setup
	//VkDescriptorSetLayoutBinding descriptor_layout_binding = {};
	//descriptor_layout_binding.binding = 0;
	//descriptor_layout_binding.descriptorCount = 1;
	//descriptor_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//// In this scenario we have the same descriptorSetLayout for both shaders...
	//// However, many times you would want seperate layouts for each since they tend to have different needs 
	//descriptor_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	//descriptor_layout_binding.pImmutableSamplers = nullptr;
	//VkDescriptorSetLayoutCreateInfo descriptor_create_info = {};
	//descriptor_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//descriptor_create_info.flags = 0; 
	//descriptor_create_info.bindingCount = 1;
	//descriptor_create_info.pBindings = &descriptor_layout_binding;
	//descriptor_create_info.pNext = nullptr;
	//// Descriptor layout
	//vkCreateDescriptorSetLayout(device, &descriptor_create_info, nullptr, &descriptorLayout);
	//// Create a descriptor pool!
	//unsigned max_frames = 0;
	//vulkan.GetSwapchainImageCount(max_frames);
	//VkDescriptorPoolCreateInfo descriptorpool_create_info = {};
	//descriptorpool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	//VkDescriptorPoolSize descriptorpool_size = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_frames };
	//descriptorpool_create_info.poolSizeCount = 1;
	//descriptorpool_create_info.pPoolSizes = &descriptorpool_size;
	//descriptorpool_create_info.maxSets = max_frames;
	//descriptorpool_create_info.flags = 0;
	//descriptorpool_create_info.pNext = nullptr;
	//vkCreateDescriptorPool(device, &descriptorpool_create_info, nullptr, &descriptorPool);
	//// Create a descriptorSet for each uniform buffer!
	//VkDescriptorSetAllocateInfo descriptorset_allocate_info = {};
	//descriptorset_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//descriptorset_allocate_info.descriptorSetCount = 1;
	//descriptorset_allocate_info.pSetLayouts = &descriptorLayout;
	//descriptorset_allocate_info.descriptorPool = descriptorPool;
	//descriptorset_allocate_info.pNext = nullptr;
	//descriptorSet.resize(max_frames);
	//for (int i = 0; i < max_frames; ++i) {
	//	vkAllocateDescriptorSets(device, &descriptorset_allocate_info, &descriptorSet[i]);
	//}
	//// link descriptor sets to uniform buffers (one for each bufferimage)
	//VkWriteDescriptorSet write_descriptorset = {};
	//write_descriptorset.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//write_descriptorset.descriptorCount = 1;
	//write_descriptorset.dstArrayElement = 0;
	//write_descriptorset.dstBinding = 0;
	//write_descriptorset.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//for (int i = 0; i < max_frames; ++i) {
	//	write_descriptorset.dstSet = descriptorSet[i];
	//	VkDescriptorBufferInfo dbinfo = { uniformHandle[i], 0, VK_WHOLE_SIZE };
	//	write_descriptorset.pBufferInfo = &dbinfo;
	//	vkUpdateDescriptorSets(device, 1, &write_descriptorset, 0, nullptr);
	//}
	//// Descriptor pipeline layout
	//VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	//pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//pipeline_layout_create_info.setLayoutCount = 1;
	//pipeline_layout_create_info.pSetLayouts = &descriptorLayout;
	//pipeline_layout_create_info.pushConstantRangeCount = 0;
	//pipeline_layout_create_info.pPushConstantRanges = nullptr;
	//vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);
	//// Pipeline State... (FINALLY) 
	//VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	//pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	//pipeline_create_info.stageCount = 2;
	//pipeline_create_info.pStages = stage_create_info;
	//pipeline_create_info.pInputAssemblyState = &assembly_create_info;
	//pipeline_create_info.pVertexInputState = &input_vertex_info;
	//pipeline_create_info.pViewportState = &viewport_create_info;
	//pipeline_create_info.pRasterizationState = &rasterization_create_info;
	//pipeline_create_info.pMultisampleState = &multisample_create_info;
	//pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
	//pipeline_create_info.pColorBlendState = &color_blend_create_info;
	//pipeline_create_info.pDynamicState = &dynamic_create_info;
	//pipeline_create_info.layout = pipelineLayout;
	//pipeline_create_info.renderPass = renderPass;
	//pipeline_create_info.subpass = 0;
	//pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	//if (VK_SUCCESS != vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
	//	&pipeline_create_info, nullptr, &pipeline))
	//	return false; // something went wrong

//	return true;
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
	/*	draw_counter = 0; */
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
	});
	// runs once per frame after updateDraw
	completeDraw = game->system<RenderingSystem>().kind(flecs::PostUpdate)
		.each([this](flecs::entity e, RenderingSystem& s) {
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

				modelID.mod_id = e.get_mut<Instance>()->transformStart;
				for (unsigned int j = 0; j < e.get_mut<Object>()->meshCount; ++j)
				{
					auto meshCount = e.get_mut<Object>()->meshStart + j;
					modelID.mat_id = levelData->levelMeshes[meshCount].materialIndex + e.get_mut<Object>()->materialStart;

					auto colorModel = e.get_mut<Material>()->diffuse.value;
					modelID.color = GW::MATH::GVECTORF{ colorModel.x, colorModel.y, colorModel.z, 1 };

					curHandles.context->UpdateSubresource(constantModelBuffer.Get(), 0, nullptr, &modelID, 0, 0);

					curHandles.context->DrawIndexedInstanced(levelData->levelMeshes[meshCount].drawInfo.indexCount,
						e.get_mut<Instance>()->transformCount,
						levelData->levelMeshes[meshCount].drawInfo.indexOffset + e.get_mut<Object>()->indexStart,
						e.get_mut<Object>()->vertexStart,
						0);

				}
		ReleasePipelineHandles(curHandles);
	});
	// NOTE: I went with multi-system approach for the ease of passing lambdas with "this"
	// There is a built-in solution for this problem referred to as a "custom runner":
	// https://github.com/SanderMertens/flecs/blob/master/examples/cpp/systems/custom_runner/src/main.cpp
	// The negative is that it requires the use of a C callback which is less flexibe vs the lambda
	// you could embed what you need in the ecs and use a lookup to get it but I think that is less clean
	
	// all drawing operations have been setup
	return true;
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
