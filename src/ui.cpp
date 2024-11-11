#include <windows.h>

// Declare the window procedure function
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Entry point for a Windows application
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"SampleWindowClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    // Register the window class
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Create the main window
    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Raw C++ Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1500, 900,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Show the window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Window procedure function
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hMemoryPanel, hTerminalPanel, hEditorPanel, hRegistersPanel, hStackPanel, hInfoPanel;

    switch (msg) {
        case WM_CREATE: {
            // Create the memory panel
            hMemoryPanel = CreateWindowExW(
                0, L"STATIC", L"[ Memory panel ]",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                10, 10, 300, 600, // Position and size of the memory panel
                hwnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // Create the terminal panel
            hTerminalPanel = CreateWindowExW(
                0, L"STATIC", L"[ Terminal panel ]\nInfo: File successfully loaded",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                320, 10, 500, 100, // Position and size of the terminal panel
                hwnd, (HMENU)2, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // Create the editor panel
            hEditorPanel = CreateWindowExW(
                0, L"STATIC", L"[ Editor panel ]\n1: load b8 10\n2: addi a8 a8 1\n3: sttl b8 a8 2\n4: halt",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                320, 120, 500, 200, // Position and size of the editor panel
                hwnd, (HMENU)3, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // Create the registers panel
            hRegistersPanel = CreateWindowExW(
                0, L"STATIC", L"[ Registers panel ]\nRegisters:\nA8: 00000000  A16: 0000000000000000\nB8: 00000000  B16: 0000000000000000\nPC: 00000000",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                830, 10, 300, 200, // Position and size of the registers panel
                hwnd, (HMENU)4, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // Create the stack panel
            hStackPanel = CreateWindowExW(
                0, L"STATIC", L"[ Stack panel ]\n00: 0x00 0x00 0x00 0x00\n10: 0x00 0x00 0x00 0x00",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                830, 220, 300, 200, // Position and size of the stack panel
                hwnd, (HMENU)5, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );

            // Create the info panel
            hInfoPanel = CreateWindowExW(
                0, L"STATIC", L"[ Info panel ]\n- Info: Press [F] to go step-by-step or [R] to run the program. Press [Q] to quit",
                WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                320, 330, 500, 100, // Position and size of the info panel
                hwnd, (HMENU)6, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}