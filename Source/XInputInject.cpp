#include <Windows.h>
#include <Xinput.h>
#include "MinHook.h"
#include <fstream>

#if defined _M_X64
#pragma comment(lib, "libMinHook-x64-v141-md.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook-x86-v141-md.lib")
#endif

#pragma comment(lib, "xinput.lib")
#pragma warning(disable:4996)

typedef DWORD(WINAPI* XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION*);
typedef DWORD(WINAPI* XINPUTGETSTATE)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* XINPUTGETCAPABILITIES)(DWORD, DWORD, XINPUT_CAPABILITIES*);

// Pointer for calling original
static XINPUTSETSTATE hookedXInputSetState = nullptr;
static XINPUTGETSTATE hookedXInputGetState = nullptr;
static XINPUTGETCAPABILITIES hookedXInputGetCapabilities = nullptr;

// Define the XINPUT_STATE_EX structure
typedef struct _XINPUT_STATE_EX {
	DWORD dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
	WORD wButtonsEx;
} XINPUT_STATE_EX, * PXINPUT_STATE_EX;

// Define the XInputGetStateEx function pointer
typedef DWORD(WINAPI* PFN_XInputGetStateEx)(DWORD dwUserIndex, XINPUT_STATE_EX* pState);

HMODULE hModule;
PFN_XInputGetStateEx pXInputGetStateEx;

std::ofstream outfile;

// wrapper for easier setting up hooks for MinHook
template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

template <typename T>
inline MH_STATUS MH_CreateHookApiEx(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHookApi(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

DWORD WINAPI detourXInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE_EX* pState)
{
	// first call the original function
	DWORD toReturn = pXInputGetStateEx(dwUserIndex, pState);
	printf("detourXInputGetStateEx %u\n", dwUserIndex);
	return toReturn;
}

//Own GetState
DWORD WINAPI detourXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	// first call the original function
	DWORD toReturn = hookedXInputGetState(dwUserIndex, pState);
	
	switch (dwUserIndex)
	{
		case 0:
		{
			// get pState from controller 1...3
			for (DWORD idx = 1; idx < XUSER_MAX_COUNT; idx++)
			{
				XINPUT_STATE_EX pState2;
				ZeroMemory(&pState2, sizeof(XINPUT_STATE_EX));

				DWORD toReturn = pXInputGetStateEx(idx, &pState2);
				if (toReturn == ERROR_SUCCESS)
				{
					pState->dwPacketNumber = pState2.dwPacketNumber;
					pState->Gamepad = pState2.Gamepad;

					printf("Swapped GetState from: %u\n", idx);
					break;
				}
				else
				{
					// do something
				}
			}
			break;
		}

		default:
		{
			// blank pState of other controllers except first controller
			ZeroMemory(pState, sizeof(XINPUT_STATE));
			return ERROR_EMPTY;
		}
	}
		
	return toReturn;
}

//Own SetState
DWORD WINAPI detourXInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
	// first call the original function
	DWORD toReturn = hookedXInputSetState(dwUserIndex, pVibration);
	printf("detourXInputSetState %u\n", dwUserIndex);

	switch (dwUserIndex)
	{
		case 0:
		{
			// get pState from controller 1...3
			for (DWORD idx = 1; idx < XUSER_MAX_COUNT; idx++)
			{
				toReturn = hookedXInputSetState(idx, pVibration);

				if (toReturn == ERROR_SUCCESS)
				{
					printf("Swapped SetState from: %u\n", idx);
					break;
				}
				else
				{
					// do something
				}
			}
			break;
		}

		default:
		{
			// blank pState of other controllers except first controller
			ZeroMemory(pVibration, sizeof(XINPUT_VIBRATION));
		}
	}

	return toReturn;
}

DWORD WINAPI detourXInputGetCapabilities(DWORD dwUserIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
	// first call the original function
	DWORD toReturn = hookedXInputGetCapabilities(dwUserIndex, flags, capabilities);
	printf("detourXInputSetState %u\n", dwUserIndex);

	switch (dwUserIndex)
	{
		case 0:
		{
			// get pState from controller 1...3
			for (DWORD idx = 1; idx < XUSER_MAX_COUNT; idx++)
			{
				XINPUT_CAPABILITIES pState2;
				ZeroMemory(&pState2, sizeof(XINPUT_CAPABILITIES));

				DWORD toReturn = hookedXInputGetCapabilities(idx, flags, &pState2);

				if (toReturn == ERROR_SUCCESS)
				{
					capabilities->Flags = pState2.Flags;
					capabilities->Gamepad = pState2.Gamepad;
					capabilities->SubType = pState2.SubType;
					capabilities->Type = pState2.Type;
					capabilities->Vibration = pState2.Vibration;

					printf("Swapped GetCapabilities from: %u\n", idx);
					break;
				}
				else
				{
					// do something
				}
			}
			break;
		}

		default:
		{
			// blank pState of other controllers except first controller
			ZeroMemory(capabilities, sizeof(XINPUT_CAPABILITIES));
		}
	}

	return toReturn;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);

	switch (ul_reason_for_call){
		case DLL_PROCESS_ATTACH: {
			printf("Attached\n");

			if (MH_Initialize() == MH_OK)
				printf("Initialized\n");

			//1_4
			if (MH_CreateHookApiEx(L"XINPUT1_4", "XInputSetState", &detourXInputSetState, &hookedXInputSetState) == MH_OK)
				if (MH_CreateHookApiEx(L"XINPUT1_4", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
				{
					printf("Detour XInputGetState 1_4\n");

					// Get a handle to the xinput library
					hModule = LoadLibrary("xinput1_4.dll");
					if (hModule != NULL)
					{
						// Get a pointer to the XInputGetStateEx function
						pXInputGetStateEx = (PFN_XInputGetStateEx)GetProcAddress(hModule, (LPCSTR)100);
						if (pXInputGetStateEx != NULL)
						{
							if (MH_CreateHookApiEx(L"XINPUT1_4", "XInputGetStateEx", &detourXInputGetStateEx, &pXInputGetStateEx) == MH_OK)
								printf("Detour XInputGetStateEx 1_4\n");
						}
					}

					//1_4
					if (MH_CreateHookApiEx(L"XINPUT1_4", "XInputGetCapabilities", &detourXInputGetCapabilities, &hookedXInputGetCapabilities) == MH_OK)
						printf("Detour XInputGetCapabilities 1_4\n");
				}

			//1_3
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT1_3", "XInputSetState", &detourXInputSetState, &hookedXInputSetState) == MH_OK)
					if (MH_CreateHookApiEx(L"XINPUT1_3", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
						printf("Detour XInputGetState 1_3\n");

			//1_2
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT1_2", "XInputSetState", &detourXInputSetState, &hookedXInputSetState) == MH_OK)
					if (MH_CreateHookApiEx(L"XINPUT_1_2", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
						printf("Detour XInputGetState 1_2\n");

			//1_1
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT1_1", "XInputSetState", &detourXInputSetState, &hookedXInputSetState) == MH_OK)
					if (MH_CreateHookApiEx(L"XINPUT_1_1", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
						printf("Detour XInputGetState 1_1\n");

			//1.0
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT9_1_0", "XInputSetState", &detourXInputSetState, &hookedXInputSetState) == MH_OK)
					if (MH_CreateHookApiEx(L"XINPUT9_1_0", "XInputGetStateEx", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
						printf("Detour XInputGetState 9_1_0\n");

			//if (MH_EnableHook(&detourXInputGetState) == MH_OK) //Not working
			if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
				printf("XInput hooked\n");

			break;
		}

		/*case DLL_THREAD_ATTACH:
			{
				MessageBox(0, "THREAD_ATTACH", "XINPUT", MB_OK);
				break;
			}

		case DLL_THREAD_DETACH:
			{
			MessageBox(0, "THREAD_DETACH", "XINPUT", MB_OK);
			break;
		}*/

		case DLL_PROCESS_DETACH: {
			//MessageBox(0, "PROCESS_DETACH", "XINPUT", MB_OK);
			MH_DisableHook(&detourXInputGetState);
			MH_Uninitialize();
			break;
		}
	}
	return TRUE;
}
