#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>

#include "common.hpp"

void setupDebug() {
	ID3D12Debug* _debug;
	assert_msg(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))), "D3D12: Failed to get debug interface");
	_debug->EnableDebugLayer();

	ID3D12Debug1* _debug1;
	assert_msg(SUCCEEDED(_debug->QueryInterface(IID_PPV_ARGS(&_debug1))), "D3D12: Failed to get debug interface 1");
	_debug1->SetEnableGPUBasedValidation(TRUE);
	_debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);

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

void d3d12Triangle(HWND hwnd, bool* running) {
	SetWindowText(hwnd, L"D3D12 Example");

	#ifdef _DEBUG
	setupDebug();
	#endif

	IDXGIFactory2* pFactory;
	UINT flags = 0;
	#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
	#endif

	assert_msg(SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&pFactory))), "D3D12: Failed to create DXGI factory");

	IDXGIAdapter1* pAdapter;
	ID3D12Device* pDevice;
	DXGI_ADAPTER_DESC1 desc;

	UINT bestAdapter = 0;
	UINT bestScore = 0;
	for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
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

	while (*running) {
		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pDevice->Release();
	pAdapter->Release();
	pFactory->Release();
}