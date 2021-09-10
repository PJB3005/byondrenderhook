#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <stdlib.h>

typedef HRESULT(WINAPI* CreateDevice_t)(IDirect3D9* Direct3D_Object, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface);

typedef HRESULT(WINAPI* Present_t)(
	IDirect3DDevice9* Direct3D_Object, 
	const RECT    *pSourceRect,
	const RECT    *pDestRect,
	HWND          hDestWindowOverride,
	const RGNDATA *pDirtyRegion);

static CreateDevice_t createDevice;
static Present_t present;
static IDirect3DDevice9* device;
static LARGE_INTEGER perfCounterFrequency;
static LARGE_INTEGER lastPresent;
static int presents;

void hook(void** addr, void* newVal)
{
	DWORD ulOldProtect = 0;
	VirtualProtect(addr, sizeof(void*), PAGE_EXECUTE_READWRITE, &ulOldProtect);
	*addr = newVal;
	VirtualProtect(addr, sizeof(void*), ulOldProtect, &ulOldProtect);
}

HRESULT WINAPI presentHook(
	IDirect3DDevice9* Direct3D_Object,
	const RECT    *pSourceRect,
	const RECT    *pDestRect,
	HWND          hDestWindowOverride,
	const RGNDATA *pDirtyRegion)
{
	presents += 1;
	// printf("[+] present.\n");

	LARGE_INTEGER qpf;
	QueryPerformanceCounter(&qpf);
	if (qpf.QuadPart - lastPresent.QuadPart > perfCounterFrequency.QuadPart)
	{
		printf("[+] FPS: %d.\n", presents);
		presents = 0;
		lastPresent = qpf;
	}

	return present(Direct3D_Object, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void hookPresent()
{
	Present_t* vtblEntry = &(device->lpVtbl->Present);
	if (present == NULL)
		present = *vtblEntry;
	hook((void**)vtblEntry, &presentHook);
}

HRESULT WINAPI createDeviceHook(
	IDirect3D9*            Direct3D_Object,
	UINT                   Adapter,
	D3DDEVTYPE             DeviceType,
	HWND                   hFocusWindow,
	DWORD                  BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9**     ppReturnedDeviceInterface)
{
	printf("[+] createDeviceHook.\n");
	
	HRESULT result = createDevice(
		Direct3D_Object,
		Adapter,
		DeviceType,
		hFocusWindow,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		pPresentationParameters,
		ppReturnedDeviceInterface);

	if (FAILED(result))
		return result;

	device = *ppReturnedDeviceInterface;

	hookPresent();

	return result;
}

DWORD WINAPI hookThread(void* _)
{
	while (1)
	{
		Sleep(10);

		if (device != NULL)
		{
			hookPresent();
		}
	}
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD   ul_reason_for_call,
	LPVOID  lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		QueryPerformanceFrequency(&perfCounterFrequency);

		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);

		printf("[+] Successfully attached to process.\n");
		IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

		CreateDevice_t* vtblEntry = &pD3D->lpVtbl->CreateDevice;
		createDevice = *vtblEntry;
		hook((void**)vtblEntry, &createDeviceHook);

		CreateThread(NULL, 0, &hookThread, NULL, 0, NULL);
		break;
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}