#include "pch.h"
#include "OsuOverlay.h"
#include "OsuGame.h"

#define FHD_MODE true

// Global Variables:
HINSTANCE hInst;                    // current instance
WCHAR szWindowClass[100];           // the main window class name
WCHAR szTitle[100];                 // The title bar text
osuGame gameStat;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

namespace
{
    std::unique_ptr<Overlay> g_game;
};

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    std::wstring file = L"C:\\Program Files (x86)\\Steam\\steam.exe";
    //ShellExecuteW(NULL, L"open", file.c_str(), NULL, NULL, SW_NORMAL);

    StringCchCopyW(szWindowClass, 11, L"OsuOverlay");
    StringCchCopyW(szTitle, 22, L"sleepy\'s osu overlay!");
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    std::wstring arg = std::wstring(lpCmdLine);

    g_game->SetName(L"py");

    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            gameStat.readMemoryOnlyFps();
            g_game->Tick(gameStat);
        }
    }

    g_game.reset();

    CoUninitialize();

    return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    g_game = std::make_unique<Overlay>();

#if FHD_MODE
    RECT wr = { 0, 0, 1600, 900 };    // set the size, but not the position
#else
    RECT wr = { 0, 0, 1280, 720 };    // set the size, but not the position
#endif

    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

    HWND hWnd = CreateWindowW(szWindowClass,
        szTitle,
        WS_BORDER | WS_MINIMIZEBOX | WS_SYSMENU, // WS_OVERLAPPEDWINDOW // WS_BORDER | WS_MINIMIZEBOX | WS_SYSMENU
        CW_USEDEFAULT, 0,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr,
        hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    // TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

    GetClientRect(hWnd, &wr);

    g_game->Initialize(hWnd, wr.right - wr.left, wr.bottom - wr.top);

    // example: 
    // gameStat.mfOsuPlayPP = std::unique_ptr<Utilities::MappingFile>(new Utilities::MappingFile("livepp_current_pp"));

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;
    // TODO: Set s_fullscreen to true if defaulting to fullscreen.

    auto game = reinterpret_cast<Overlay*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_PAINT:
        if (s_in_sizemove && game)
        {
            game->Tick(gameStat);
        }
        else
        {
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_QUIT:
        DestroyWindow(hWnd);
        break;
    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}