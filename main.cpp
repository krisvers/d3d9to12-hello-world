#include <Windows.h>
#include "common.hpp"

void d3d9Triangle(HWND hwnd, bool* running);
void d3d10Triangle(HWND hwnd, bool* running);
void d3d11Triangle(HWND hwnd, bool* running);
void d3d12Triangle(HWND hwnd, bool* running);

static bool g_running = true;

static LRESULT WINAPI hwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main() {
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, hwndProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		L"D3D", NULL
	};

	RegisterClassEx(&wc);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 800;
	rect.bottom = 600;

	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindow(wc.lpszClassName, L"D3D", WS_OVERLAPPEDWINDOW, 100, 100, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);
	if (hwnd == NULL) {
		return 0;
	}

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	d3d9Triangle(hwnd, &g_running);
	g_running = true;
	d3d10Triangle(hwnd, &g_running);
	g_running = true;
	d3d11Triangle(hwnd, &g_running);
	g_running = true;
	d3d12Triangle(hwnd, &g_running);
	g_running = true;

	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

static LRESULT WINAPI hwndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CLOSE:
			g_running = false;
			return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}