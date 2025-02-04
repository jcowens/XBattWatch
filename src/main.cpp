#include <windows.h>
#include <xinput.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include "../include/resource.h"

#pragma comment(lib, "XInput.lib")

#define WM_TRAYICON (WM_APP + 1)
constexpr auto IDM_EXIT = 1001;
constexpr auto IDM_SELECT_CONTROLLER_0 = 2000;
constexpr auto IDM_SELECT_CONTROLLER_1 = 2001;
constexpr auto IDM_SELECT_CONTROLLER_2 = 2002;
constexpr auto IDM_SELECT_CONTROLLER_3 = 2003;
constexpr auto IDM_TURNOFF_CONTROLLER_0 = 3000;
constexpr auto IDM_TURNOFF_CONTROLLER_1 = 3001;
constexpr auto IDM_TURNOFF_CONTROLLER_2 = 3002;
constexpr auto IDM_TURNOFF_CONTROLLER_3 = 3003;

HINSTANCE hXInputDLL = NULL;
typedef DWORD(WINAPI* XInputPowerOffController_t)(DWORD i);
XInputPowerOffController_t realXInputPowerOffController = NULL;

int selectedControllerIndex = -1;
NOTIFYICONDATA nid = {};
HWND hWnd = NULL;

void UpdateTrayIcon();
void ShowContextMenu(HWND hWnd);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hXInputDLL = LoadLibraryA("XInput1_3.dll");
    if (hXInputDLL) {
        realXInputPowerOffController = (XInputPowerOffController_t)GetProcAddress(hXInputDLL, (LPCSTR)103);
    }

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"XboxControllerTrayApp";
    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(0, wc.lpszClassName, L"Xbox Controller Monitor", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    LoadIcon(hInstance, MAKEINTRESOURCE(101));
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetTimer(hWnd, 1, 30000, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hXInputDLL) FreeLibrary(hXInputDLL);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TIMER:
        UpdateTrayIcon();
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) ShowContextMenu(hWnd);
        break;
    case WM_COMMAND: {
        int cmd = LOWORD(wParam);
        if (cmd == IDM_EXIT) {
            DestroyWindow(hWnd);
        }
        else if (cmd >= IDM_SELECT_CONTROLLER_0 && cmd <= IDM_SELECT_CONTROLLER_3) {
            selectedControllerIndex = cmd - IDM_SELECT_CONTROLLER_0;
            UpdateTrayIcon();
        }
        else if (cmd >= IDM_TURNOFF_CONTROLLER_0 && cmd <= IDM_TURNOFF_CONTROLLER_3) {
            int idx = cmd - IDM_TURNOFF_CONTROLLER_0;
            if (realXInputPowerOffController) realXInputPowerOffController(idx);
            XINPUT_STATE state;
            if (XInputGetState(selectedControllerIndex, &state) != ERROR_SUCCESS)
                selectedControllerIndex = -1;
            UpdateTrayIcon();
        }
        break;
    }
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void UpdateTrayIcon() {
    if (selectedControllerIndex == -1) {
        for (int i = 0; i < 4; i++) {
            XINPUT_STATE state;
            if (XInputGetState(i, &state) == ERROR_SUCCESS) {
                selectedControllerIndex = i;
                break;
            }
        }
    }

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_BATTERY_EMPTY));
    WCHAR tip[128] = L"No controllers connected";

    if (selectedControllerIndex != -1) {
        XINPUT_BATTERY_INFORMATION batteryInfo;
        if (XInputGetBatteryInformation(selectedControllerIndex, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo) == ERROR_SUCCESS) {
            int iconId = IDI_BATTERY_EMPTY;
            switch (batteryInfo.BatteryLevel) {
            case BATTERY_LEVEL_FULL:
                iconId = IDI_BATTERY_FULL;
                wcscpy_s(tip, L"Battery: Full");
                break;
            case BATTERY_LEVEL_MEDIUM:
                iconId = IDI_BATTERY_MEDIUM;
                wcscpy_s(tip, L"Battery: Medium");
                break;
            case BATTERY_LEVEL_LOW:
                iconId = IDI_BATTERY_LOW;
                wcscpy_s(tip, L"Battery: Low");
                break;
            default:
                iconId = IDI_BATTERY_EMPTY;
                wcscpy_s(tip, L"Battery: Empty");
                break;
            }

            if (batteryInfo.BatteryType == BATTERY_TYPE_WIRED) {
                iconId = IDI_BATTERY_FULL;
                wcscpy_s(tip, L"Wired Controller");
            }

            hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconId));
        }
        else {
            selectedControllerIndex = -1;
            UpdateTrayIcon();
            return;
        }
    }

    nid.hIcon = hIcon;
    wcscpy_s(nid.szTip, tip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void ShowContextMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    HMENU hSelect = CreatePopupMenu();
    HMENU hTurnOff = CreatePopupMenu();

    for (int i = 0; i < 4; i++) {
        XINPUT_STATE state;
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            WCHAR text[32];
            swprintf_s(text, L"Controller %d", i + 1);
            AppendMenu(hSelect, MF_STRING, IDM_SELECT_CONTROLLER_0 + i, text);
            swprintf_s(text, L"Turn Off Controller %d", i + 1);
            AppendMenu(hTurnOff, MF_STRING, IDM_TURNOFF_CONTROLLER_0 + i, text);
        }
    }

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSelect, L"Select Controller");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hTurnOff, L"Turn Off Controller");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(hSelect);
    DestroyMenu(hTurnOff);
    DestroyMenu(hMenu);
}