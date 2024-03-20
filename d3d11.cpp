#include <d3d11.h>
#include <dxgi.h>

void d3d11Triangle(HWND hwnd, bool* running) {
	SetWindowText(hwnd, L"D3D11 Example");

	while (*running) {
		MSG msg;
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}