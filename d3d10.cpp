#include <d3d10.h>
#include <exception>

struct vertex_t {
	FLOAT x, y, z;
	FLOAT r, g, b;
};

void d3d10Triangle(HWND hwnd, bool* running) {
	SetWindowText(hwnd, L"D3D10 Example");

	DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
	swapchainDesc.BufferCount = 1;
	swapchainDesc.BufferDesc.Width = 800;
	swapchainDesc.BufferDesc.Height = 600;
	swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.OutputWindow = hwnd;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.Windowed = TRUE;

	IDXGISwapChain* pSwapchain;
	ID3D10Device* pDevice;
	if (FAILED(D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapchainDesc, &pSwapchain, &pDevice))) {
		throw std::exception("D3D10: Failed to create device and swapchain");
	}

	ID3D10Texture2D* pBackBuffer;
	if (FAILED(pSwapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer))) {
		throw std::exception("D3D10: Failed to get back buffer");
	}

	ID3D10RenderTargetView* pRenderTargetView;
	if (FAILED(pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView))) {
		throw std::exception("D3D10: Failed to create render target view");
	}

	pBackBuffer->Release();
	pDevice->OMSetRenderTargets(1, &pRenderTargetView, NULL);

	RECT hwndRect;
	GetClientRect(hwnd, &hwndRect);

	D3D10_VIEWPORT viewport = {};
	viewport.Width = hwndRect.right - hwndRect.left;
	viewport.Height = hwndRect.bottom - hwndRect.top;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	pDevice->RSSetViewports(1, &viewport);

	char* shaderSource;
	{
		HANDLE file = CreateFile(L"d3d10.hlsl", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) {
			throw std::exception("D3D10: Failed to open \"d3d10.hlsl\"");
		}

		DWORD fileSize = GetFileSize(file, NULL);
		shaderSource = (char*)malloc(fileSize + 1);
		if (shaderSource == NULL) {
			CloseHandle(file);
			throw std::exception("D3D10: Failed to allocate memory for \"d3d10.hlsl\"");
		}

		if (!ReadFile(file, shaderSource, fileSize, NULL, NULL)) {
			free(shaderSource);
			CloseHandle(file);
			throw std::exception("D3D10: Failed to read \"d3d10.hlsl\"");
		}

		shaderSource[fileSize] = 0;
		CloseHandle(file);
	}
	
	ID3D10Blob* vertexBlob;
	ID3D10Blob* pixelBlob;
	ID3D10Blob* errorBlob;
	
	ID3D10VertexShader* pVertexShader;
	ID3D10PixelShader* pPixelShader;

	if (FAILED(D3D10CompileShader(shaderSource, strlen(shaderSource), "d3d10.hlsl", nullptr, nullptr, "vsmain", "vs_4_0", 0, &vertexBlob, &errorBlob))) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
		throw std::exception("D3D10: Failed to compile vertex shader in \"d3d10.hlsl\"");
	}

	if (FAILED(pDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &pVertexShader))) {
		vertexBlob->Release();
		throw std::exception("D3D10: Failed to create vertex shader from \"d3d10.hlsl\"");
	}

	if (FAILED(D3D10CompileShader(shaderSource, strlen(shaderSource), "d3d10.hlsl", nullptr, nullptr, "psmain", "ps_4_0", 0, &pixelBlob, &errorBlob))) {
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
		throw std::exception("D3D10: Failed to compile pixel shader in \"d3d10.hlsl\"");
	}

	if (FAILED(pDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), &pPixelShader))) {
		pixelBlob->Release();
		throw std::exception("D3D10: Failed to create pixel shader from \"d3d10.hlsl\"");
	}

	pDevice->VSSetShader(pVertexShader);
	pDevice->PSSetShader(pPixelShader);

	D3D10_INPUT_ELEMENT_DESC layout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, sizeof(float) * 3, D3D10_INPUT_PER_VERTEX_DATA, 0
		}
	};

	ID3D10InputLayout* pVertexLayout;
	if (FAILED(pDevice->CreateInputLayout(layout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &pVertexLayout))) {
		throw std::exception("D3D10: Failed to create input layout");
	}

	vertexBlob->Release();
	pixelBlob->Release();
	pDevice->IASetInputLayout(pVertexLayout);

	vertex_t vertices[] = {
		{  0.0f,  0.5f, 0.0f,	0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f,	1.0f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f,	1.0f, 1.0f, 0.0f },
	};

	D3D10_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	
	D3D10_SUBRESOURCE_DATA data = {};
	data.pSysMem = vertices;
	data.SysMemPitch = sizeof(vertices);
	data.SysMemSlicePitch = 0;

	ID3D10Buffer* vertexBuffer;
	if (FAILED(pDevice->CreateBuffer(&bufferDesc, &data, &vertexBuffer))) {
		throw std::exception("D3D10: Failed to create vertex buffer");
	}

	UINT stride = sizeof(vertex_t);
	UINT offset = 0;
	pDevice->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D10_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D10_FILL_SOLID;
	rasterizerDesc.CullMode = D3D10_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = false;

	ID3D10RasterizerState* pRasterizerState;
	if (FAILED(pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState))) {
		throw std::exception("D3D10: Failed to create rasterizer state");
	}

	pDevice->RSSetState(pRasterizerState);

	while (*running) {
		float clearColor[4] = { 0.1f, 0.05f, 0.12f, 1.0f };
		pDevice->ClearRenderTargetView(pRenderTargetView, clearColor);

		pDevice->Draw(3, 0);

		pSwapchain->Present(0, 0);

		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	pRenderTargetView->Release();
	pVertexShader->Release();
	pPixelShader->Release();
	pVertexLayout->Release();
	vertexBuffer->Release();
	pRasterizerState->Release();
	pSwapchain->Release();
	pDevice->Release();
}