#include <d3d9.h>
#include <string.h>

struct vertex_t {
	FLOAT x, y, z, rhw;
	DWORD color;
};

void d3d9Triangle(HWND hwnd, bool* running) {
	IDirect3D9* pD3D = nullptr;
	IDirect3DDevice9* pD3DDevice = nullptr;

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == nullptr) {
		return;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice))) {
		return;
	}

	vertex_t vertices[] = {
		{ 400, 150, 0, 1.0f,	0xFF00FFFF },
		{ 600, 450, 0, 1.0f,	0xFFFFFF00 },
		{ 200, 450, 0, 1.0f,	0xFFFF00FF },
	};

	IDirect3DVertexBuffer9* pVB;
	if (FAILED(pD3DDevice->CreateVertexBuffer(3 * sizeof(vertex_t), 0, D3DFVF_XYZRHW, D3DPOOL_DEFAULT, &pVB, NULL))) {
		return;
	}

	void* pVertices;
	if (FAILED(pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0))) {
		return;
	}
	memcpy(pVertices, vertices, sizeof(vertices));
	pVB->Unlock();

	while (*running) {
		pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 40), 1.0f, 0);
		if (SUCCEEDED(pD3DDevice->BeginScene())) {
			pD3DDevice->SetStreamSource(0, pVB, 0, sizeof(vertex_t));
			pD3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
			pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 1);
			pD3DDevice->EndScene();
		}

		pD3DDevice->Present(NULL, NULL, NULL, NULL);

		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	pVB->Release();
	pD3DDevice->Release();
	pD3D->Release();
}