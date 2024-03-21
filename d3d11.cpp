#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#include "common.hpp"

struct vertex_t {
	FLOAT x, y, z;
	FLOAT r, g, b;
};

void setupDebug(ID3D11Device* device) {
	ID3D11InfoQueue* info;
	assert_msg(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&info)), "D3D11: Failed to get info queue");
	info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
	info->Release();

	HMODULE dxgidebugDLL = LoadLibraryA("dxgidebug.dll");
	if (dxgidebugDLL != nullptr) {
		HRESULT (WINAPI* DXGIGetDebugInterface)(REFIID, void**) = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(GetProcAddress(dxgidebugDLL, "DXGIGetDebugInterface"));

		IDXGIInfoQueue* dxgiInfo;
		DXGIGetDebugInterface(__uuidof(IDXGIInfoQueue), (void**)&dxgiInfo);
		dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		dxgiInfo->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
		dxgiInfo->Release();
	}
}

void debugCleanup(ID3D11Device* device) {
	ID3D11Debug* debug;
	assert_msg(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**) &debug)), "D3D11: Failed to get debug interface");
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	debug->Release();
}

IDXGISwapChain1* createSwapchain(HWND hwnd, ID3D11Device* device) {
	IDXGIDevice* dxgiDevice;
	assert_msg(SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice)), "D3D11: Failed to get DXGI device");

	IDXGIAdapter* dxgiAdapter;
	assert_msg(SUCCEEDED(dxgiDevice->GetAdapter(&dxgiAdapter)), "D3D11: Failed to get DXGI adapter");

	IDXGIFactory2* dxgiFactory;
	assert_msg(SUCCEEDED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory)), "D3D11: Failed to get DXGI factory");

	RECT hwndRect;
	GetClientRect(hwnd, &hwndRect);

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.Width = hwndRect.right - hwndRect.left;
	desc.Height = hwndRect.bottom - hwndRect.top;
	desc.BufferCount = 2;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = 0;

	IDXGISwapChain1* pSwapchain;
	assert_msg(SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, &pSwapchain)), "D3D11: Failed to create swapchain");
	dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();
	dxgiAdapter->Release();
	dxgiDevice->Release();

	return pSwapchain;
}

void d3d11Triangle(HWND hwnd, bool* running) {
	SetWindowText(hwnd, L"D3D11 Example");

	ID3D11Device* pDevice = nullptr;
	ID3D11DeviceContext* pDeviceContext = nullptr;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	UINT creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	assert_msg(SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, &featureLevel, 1, D3D11_SDK_VERSION, &pDevice, NULL, &pDeviceContext)), "D3D11: device creation failed");

#ifdef _DEBUG
	setupDebug(pDevice);
#endif

	IDXGISwapChain1* pSwapchain = createSwapchain(hwnd, pDevice);

	ID3D11Buffer* pVertexBuffer;

	vertex_t vertices[] = {
		{  0.0f,  0.5f, 0.0f,	0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f,	1.0f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f,	1.0f, 1.0f, 0.0f },
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = vertices;

	assert_msg(SUCCEEDED(pDevice->CreateBuffer(&bufferDesc, &subresourceData, &pVertexBuffer)), "D3D11: Failed to create vertex buffer");

	ID3D11VertexShader* pVertexShader;
	ID3D11PixelShader* pPixelShader;

	char* shaderSource;
	{
		HANDLE file = CreateFile(L"d3d11.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		assert_msg(file != INVALID_HANDLE_VALUE, "D3D11: Failed to open \"d3d11.hlsl\"");

		DWORD fileSize = GetFileSize(file, NULL);
		shaderSource = new char[fileSize + 1];

		assert_msg_action(shaderSource != NULL,
			"D3D11: Failed to allocate memory for \"d3d11.hlsl\"",
			CloseHandle(file));

		assert_msg_action(ReadFile(file, shaderSource, fileSize, NULL, NULL),
			"D3D11: Failed to read \"d3d11.hlsl\"",
			delete[] shaderSource; CloseHandle(file));

		shaderSource[fileSize] = 0;
		CloseHandle(file);
	}

	ID3DBlob* vertexBlob;
	ID3DBlob* pixelBlob;
	ID3DBlob* errorBlob;

	assert_msg_action(SUCCEEDED(D3DCompile(shaderSource, strlen(shaderSource), "d3d11.hlsl", nullptr, nullptr, "vsmain", "vs_5_0", 0, 0, &vertexBlob, &errorBlob)),
		"D3D11: Failed to compile vertex shader in \"d3d11.hlsl\"",
		OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release());

	assert_msg(SUCCEEDED(pDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &pVertexShader)), "D3D11: Failed to create vertex shader from \"d3d11.hlsl\"");

	assert_msg_action(SUCCEEDED(D3DCompile(shaderSource, strlen(shaderSource), "d3d11.hlsl", nullptr, nullptr, "psmain", "ps_5_0", 0, 0, &pixelBlob, &errorBlob)),
		"D3D11: Failed to compile pixel shader in \"d3d11.hlsl\"",
		OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release());

	assert_msg(SUCCEEDED(pDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pPixelShader)), "D3D11: Failed to create pixel shader from \"d3d11.hlsl\"");

	ID3D11InputLayout* pInputLayout;
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(vertex_t, r), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	assert_msg(SUCCEEDED(pDevice->CreateInputLayout(layout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &pInputLayout)), "D3D11: Failed to create input layout");

	vertexBlob->Release();
	pixelBlob->Release();

	UINT offset = 0;
	UINT stride = sizeof(vertex_t);

	RECT hwndRect;
	GetClientRect(hwnd, &hwndRect);

	D3D11_VIEWPORT viewport = {};
	viewport.Width = hwndRect.right - hwndRect.left;
	viewport.Height = hwndRect.bottom - hwndRect.top;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;

	ID3D11RasterizerState* pRasterizerState;
	assert_msg(SUCCEEDED(pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState)), "D3D11: Failed to create rasterizer state");

	ID3D11RenderTargetView* pRenderTargetView;
	ID3D11Texture2D* pBackBuffer;

	assert_msg(SUCCEEDED(pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)), "D3D11: Failed to get back buffer");
	assert_msg(SUCCEEDED(pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView)), "D3D11: Failed to create render target view");

	pBackBuffer->Release();

	while (*running) {
		float clearColor[4] = { 0.09f, 0.14f, 0.05f, 1.0f };
		pDeviceContext->ClearRenderTargetView(pRenderTargetView, clearColor);

		pDeviceContext->IASetInputLayout(pInputLayout);
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

		pDeviceContext->VSSetShader(pVertexShader, nullptr, 0);
		pDeviceContext->PSSetShader(pPixelShader, nullptr, 0);

		pDeviceContext->RSSetViewports(1, &viewport);
		pDeviceContext->RSSetState(pRasterizerState);

		pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

		pDeviceContext->Draw(3, 0);

		pSwapchain->Present(1, 0);

		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pRasterizerState->Release();
	pInputLayout->Release();
	pPixelShader->Release();
	pVertexShader->Release();
	pVertexBuffer->Release();
	pRenderTargetView->Release();
	pSwapchain->Release();
	pDeviceContext->Release();
	debugCleanup(pDevice);
	pDevice->Release();
}