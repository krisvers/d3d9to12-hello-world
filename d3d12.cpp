#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#include "common.hpp"

#define D3D12_VALIDATION
#define BACKBUFFER_COUNT 2

struct vertex_t {
	FLOAT x, y, z;
	FLOAT r, g, b;
};

void setupDebug() {
	ID3D12Debug* _debug;
	assert_msg(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))), "D3D12: Failed to get debug interface");
	_debug->EnableDebugLayer();

	ID3D12Debug1* _debug1;
	assert_msg(SUCCEEDED(_debug->QueryInterface(IID_PPV_ARGS(&_debug1))), "D3D12: Failed to get debug interface 1");
	#ifdef D3D12_VALIDATION
	_debug1->SetEnableGPUBasedValidation(TRUE);
	_debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
	#endif

	_debug1->Release();
	_debug->Release();

	IDXGIDebug* _dxgiDebug;
	assert_msg(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&_dxgiDebug))), "D3D12: Failed to get DXGI debug interface");
	_dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

	IDXGIInfoQueue* _dxgiInfo;
	assert_msg(SUCCEEDED(_dxgiDebug->QueryInterface(IID_PPV_ARGS(&_dxgiInfo))), "D3D12: Failed to get DXGI info queue");
	_dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	_dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);

	_dxgiInfo->Release();
	_dxgiDebug->Release();
}

IDXGISwapChain4* createSwapChain(HWND hwnd, IDXGIFactory4* pFactory, ID3D12CommandQueue* pCommandQueue) {
	RECT hwndRect;
	GetClientRect(hwnd, &hwndRect);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = hwndRect.right - hwndRect.left;
	swapChainDesc.Height = hwndRect.bottom - hwndRect.top;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BACKBUFFER_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	IDXGISwapChain4* pSwapChain;
	IDXGISwapChain1* pSwapChain1;
	assert_msg(SUCCEEDED(pFactory->CreateSwapChainForHwnd(pCommandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &pSwapChain1)), "D3D12: Failed to create swap chain");
	assert_msg(SUCCEEDED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain))), "D3D12: Failed to get swap chain");

	pSwapChain1->Release();

	return pSwapChain;
}

void d3d12Triangle(HWND hwnd, bool* running) {
	SetWindowText(hwnd, L"D3D12 Example");

	#ifdef _DEBUG
	setupDebug();
	#endif

	IDXGIFactory4* pFactory;
	UINT flags = 0;
	#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
	#endif

	assert_msg(SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&pFactory))), "D3D12: Failed to create DXGI factory");

	IDXGIAdapter1* pAdapter;
	ID3D12Device* pDevice;

	UINT bestAdapter = 0;
	UINT bestScore = 0;
	for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		UINT score = 0;
		if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
			score += 5000;
		}

		score += static_cast<UINT>(desc.DedicatedVideoMemory / 1024);

		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)))) {
			if (score > bestScore) {
				bestScore = score;
				bestAdapter = i;
			}

			pDevice->Release();
		}

		pAdapter->Release();
	}

	if (bestScore == 0) {
		assert_msg(false, "D3D12: Failed to find suitable adapter");
	}

	assert_msg(SUCCEEDED(pFactory->EnumAdapters1(bestAdapter, &pAdapter)), "D3D12: Failed to get adapter");
	assert_msg(SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice))), "D3D12: Failed to create device");

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	ID3D12CommandQueue* pCommandQueue;
	assert_msg(SUCCEEDED(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&pCommandQueue))), "D3D12: Failed to create command queue");

	IDXGISwapChain4* pSwapChain = createSwapChain(hwnd, pFactory, pCommandQueue);

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = BACKBUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ID3D12DescriptorHeap* pRtvHeap;
	assert_msg(SUCCEEDED(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pRtvHeap))), "D3D12: Failed to create RTV descriptor heap");

	UINT rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	ID3D12Resource* pBackBuffers[BACKBUFFER_COUNT];
	for (UINT i = 0; i < BACKBUFFER_COUNT; ++i) {
		assert_msg(SUCCEEDED(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffers[i]))), "D3D12: Failed to get back buffer");
		pDevice->CreateRenderTargetView(pBackBuffers[i], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	ID3D12CommandAllocator* pCommandAllocator;
	assert_msg(SUCCEEDED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator))), "D3D12: Failed to create command allocator");

	ID3D12GraphicsCommandList* pCommandList;
	assert_msg(SUCCEEDED(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList))), "D3D12: Failed to create command list");

	pCommandList->Close();

	ID3D12Fence* pFence;
	assert_msg(SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence))), "D3D12: Failed to create fence");

	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert_msg(fenceEvent != nullptr, "D3D12: Failed to create fence event");

	UINT64 fenceValue = 1;

	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 800;
	viewport.Height = 600;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = 800;
	scissorRect.bottom = 600;

	vertex_t vertices[] = {
		{  0.0f,  0.5f, 0.0f,	0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f,	1.0f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f,	1.0f, 1.0f, 0.0f },
	};

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(vertices);
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* pUploadBuffer;
	assert_msg(SUCCEEDED(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer))), "D3D12: Failed to create upload buffer");

	void* pMappedData;
	assert_msg(SUCCEEDED(pUploadBuffer->Map(0, nullptr, &pMappedData)), "D3D12: Failed to map upload buffer");
	memcpy(pMappedData, vertices, sizeof(vertices));
	pUploadBuffer->Unmap(0, nullptr);

	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	ID3D12Resource* pVertexBuffer;
	assert_msg(SUCCEEDED(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pVertexBuffer))), "D3D12: Failed to create vertex buffer");

	pCommandList->Reset(pCommandAllocator, nullptr);
	pCommandList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, sizeof(vertices));

	D3D12_RESOURCE_BARRIER resourceBarrier = {};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Transition.pResource = pVertexBuffer;
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	pCommandList->ResourceBarrier(1, &resourceBarrier);
	pCommandList->Close();

	pCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&pCommandList));
	pCommandQueue->Signal(pFence, fenceValue);

	if (pFence->GetCompletedValue() < fenceValue) {
		pFence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_SHADER_BYTECODE vertexShaderBytecode;
	D3D12_SHADER_BYTECODE pixelShaderBytecode;

	ID3DBlob* vertexShaderBlob;
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* errorBlob;

	assert_msg(SUCCEEDED(D3DCompileFromFile(L"d3d12.hlsl", nullptr, nullptr, "vsmain", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob)), "D3D12: Failed to compile vertex shader");
	assert_msg(SUCCEEDED(D3DCompileFromFile(L"d3d12.hlsl", nullptr, nullptr, "psmain", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob)), "D3D12: Failed to compile pixel shader");

	vertexShaderBytecode.BytecodeLength = vertexShaderBlob->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShaderBlob->GetBufferPointer();

	pixelShaderBytecode.BytecodeLength = pixelShaderBlob->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShaderBlob->GetBufferPointer();

	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
	rootSignatureDesc.Desc_1_0.NumParameters = 0;
	rootSignatureDesc.Desc_1_0.pParameters = nullptr;
	rootSignatureDesc.Desc_1_0.NumStaticSamplers = 0;
	rootSignatureDesc.Desc_1_0.pStaticSamplers = nullptr;
	rootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rootSignatureBlob;
	assert_msg(SUCCEEDED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &rootSignatureBlob, &errorBlob)), "D3D12: Failed to serialize root signature");

	ID3D12RootSignature* pRootSignature;
	assert_msg(SUCCEEDED(pDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature))), "D3D12: Failed to create root signature");

	rootSignatureBlob->Release();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.pRootSignature = pRootSignature;
	pipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	pipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	pipelineStateDesc.StreamOutput = { nullptr, 0, nullptr, 0, 0 };
	pipelineStateDesc.BlendState = blendDesc;
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.RasterizerState = rasterizerDesc;
	pipelineStateDesc.InputLayout = { inputElementDescs, 2 };
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;

	ID3D12PipelineState* pPipelineState;
	assert_msg(SUCCEEDED(pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pPipelineState))), "D3D12: Failed to create pipeline state");

	UINT frameIndex = pSwapChain->GetCurrentBackBufferIndex();
	while (*running) {
		pCommandAllocator->Reset();
		pCommandList->Reset(pCommandAllocator, pPipelineState);

		pCommandList->SetGraphicsRootSignature(pRootSignature);
		pCommandList->SetPipelineState(pPipelineState);

		resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resourceBarrier.Transition.pResource = pBackBuffers[frameIndex];
		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        pCommandList->ResourceBarrier(1, &resourceBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += frameIndex * rtvDescriptorSize;

		pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		FLOAT clearColor[] = { 0.3f, 0.4f, 0.25f, 1.0f };
		pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		pCommandList->RSSetViewports(1, &viewport);
		pCommandList->RSSetScissorRects(1, &scissorRect);

		pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(vertex_t);
		vertexBufferView.SizeInBytes = sizeof(vertices);

		pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		pCommandList->DrawInstanced(3, 1, 0, 0);

		resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		pCommandList->ResourceBarrier(1, &resourceBarrier);

		pCommandList->Close();

		pCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&pCommandList));

		pSwapChain->Present(1, 0);

		pCommandQueue->Signal(pFence, fenceValue);

		if (pFence->GetCompletedValue() < fenceValue) {
			pFence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		++fenceValue;

		frameIndex = pSwapChain->GetCurrentBackBufferIndex();

		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CloseHandle(fenceEvent);
	pFence->Release();
	pVertexBuffer->Release();
	pUploadBuffer->Release();
	pCommandList->Release();
	pCommandAllocator->Release();
	pPipelineState->Release();
	pRootSignature->Release();
	for (UINT i = 0; i < BACKBUFFER_COUNT; ++i) {
		pBackBuffers[i]->Release();
	}
	pRtvHeap->Release();
	pSwapChain->Release();
	pCommandQueue->Release();

	pDevice->Release();
	pAdapter->Release();
	pFactory->Release();
}