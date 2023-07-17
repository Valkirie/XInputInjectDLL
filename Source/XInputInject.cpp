﻿#include <Windows.h>
//#include <Xinput.h>
#include "MinHook.h"
#include <fstream>

#if defined _M_X64
#pragma comment(lib, "libMinHook-x64-v141-md.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook-x86-v141-md.lib")
#endif

#pragma comment(lib, "xinput.lib")

#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y				0x8000

#define BATTERY_TYPE_DISCONNECTED		0x00

#define XUSER_MAX_COUNT                 4
#define XUSER_INDEX_ANY					0x000000FF

#define ERROR_DEVICE_NOT_CONNECTED		1167
#define ERROR_SUCCESS					0

typedef struct _XINPUT_GAMEPAD
{
	WORD                                wButtons;
	BYTE                                bLeftTrigger;
	BYTE                                bRightTrigger;
	SHORT                               sThumbLX;
	SHORT                               sThumbLY;
	SHORT                               sThumbRX;
	SHORT                               sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE
{
	DWORD                               dwPacketNumber;
	XINPUT_GAMEPAD                      Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;


typedef DWORD(WINAPI *XINPUTGETSTATE)(DWORD, XINPUT_STATE*);

// Pointer for calling original
static XINPUTGETSTATE hookedXInputGetState = nullptr;

std::ofstream outfile;

XINPUT_STATE pOthersState;


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

//Own GetState
DWORD WINAPI detourXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
	//MessageBox(0, "XinputGetSate", "XINPUT", MB_OK);
	
	//ZeroMemory(pState, sizeof(XINPUT_STATE));

	// first call the original function
	DWORD toReturn = hookedXInputGetState(dwUserIndex, pState);
	
	switch (dwUserIndex) {
		case 0: {
			XINPUT_STATE p2state;
			ZeroMemory(&p2state, sizeof(XINPUT_STATE));

			// get pState from controller 2
			DWORD result = hookedXInputGetState(1, &p2state);
			
			pState->Gamepad = p2state.Gamepad;
		break;
		}
		default: {
			XINPUT_STATE pStateBlank;
			ZeroMemory(&pStateBlank, sizeof(XINPUT_STATE));

			// blank pState of other controllers except first controller
			pState->Gamepad = pStateBlank.Gamepad;
		}
	}
	//Any pState change

	//Sample 1
	//pState->Gamepad.sThumbRX = 0;
	//pState->Gamepad.sThumbRY = 0;

	
	//pState change
	//pState->Gamepad.sThumbRX = 0;
	//pState->Gamepad.sThumbRY = 0;

	//if (pState->Gamepad.bLeftTrigger > 200) {
		//pState->Gamepad.wButtons = 0x1000;
		//State->Gamepad.bLeftTrigger = 0;
		
	return toReturn;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call){
		case DLL_PROCESS_ATTACH: {
			MessageBox(0, "Attached", "XINPUT", MB_OK);

			if (MH_Initialize() == MH_OK)
				MessageBox(0, "Initialized ok", "XINPUT", MB_OK);

			//1.0
			if (MH_CreateHookApiEx(L"XINPUT9_1_0", "XInputGetStateEx", &detourXInputGetState, &hookedXInputGetState) == MH_OK) //Ex! - Bulletstorm (2011)
				MessageBox(0, "Detour XInputGetState 9_1_0", "XINPUT", MB_OK);

			//1_1
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT_1_1", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
					MessageBox(0, "Detour XInputGetState 1_1", "XINPUT", MB_OK);

			//1_2
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT_1_2", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
					MessageBox(0, "Detour XInputGetState 1_2", "XINPUT", MB_OK);


			//1_3
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT1_3", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
					MessageBox(0, "Detour XInputGetState 1_3", "XINPUT", MB_OK);

			//1_4
			if (hookedXInputGetState == nullptr)
				if (MH_CreateHookApiEx(L"XINPUT1_4", "XInputGetState", &detourXInputGetState, &hookedXInputGetState) == MH_OK)
					MessageBox(0, "Detour XInputGetState 1_4", "XINPUT", MB_OK);


			//if (MH_EnableHook(&detourXInputGetState) == MH_OK) //Not working
			if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
				MessageBox(0, "XInput hooked", "XINPUT", MB_OK);


			/*
			// Load the XInput library dynamically
			HMODULE hXInput = LoadLibrary("xinput1_4.dll");
			if (!hXInput) {
				hXInput = LoadLibrary("xinput1_3.dll");
			}
			if (!hXInput) {
				MessageBox(0, "Failed hXInput!", "XINPUT", MB_OK);
				return FALSE;
			}
			MessageBox(0, "Loaded hXInput!", "XINPUT", MB_OK);

			// Get the XInputGetState function address
			XINPUTGETSTATE XInputGetState = (XINPUTGETSTATE)GetProcAddress(hXInput, "XInputGetState");
			
			if (!XInputGetState) {
				return FALSE;
			}
			*/
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
