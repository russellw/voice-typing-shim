#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>

static HWND g_hEdit = NULL;
static HWND g_hButton = NULL;

#define ID_EDIT   1
#define ID_COPY   2
#define BTN_H     40

static void TriggerVoiceTyping()
{
    // Simulate Win+H to invoke Windows voice typing
    INPUT inputs[4] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'H';

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'H';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
}

static void CopyEditToClipboard(HWND hWnd)
{
    int len = GetWindowTextLength(g_hEdit);
    if (len == 0)
        return;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(WCHAR));
    if (!hMem)
        return;

    WCHAR *buf = (WCHAR *)GlobalLock(hMem);
    GetWindowText(g_hEdit, buf, len + 1);
    GlobalUnlock(hMem);

    if (OpenClipboard(hWnd))
    {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
    else
    {
        GlobalFree(hMem);
    }
}

static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam, UINT_PTR uIdSubclass,
                                          DWORD_PTR dwRefData)
{
    (void)uIdSubclass; (void)dwRefData;
    if (msg == WM_SETFOCUS)
        TriggerVoiceTyping();
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        g_hButton = CreateWindowEx(
            0, L"BUTTON", L"Copy (F12)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, rc.bottom - BTN_H, rc.right, BTN_H,
            hWnd, (HMENU)ID_COPY, GetModuleHandle(NULL), NULL);

        g_hEdit = CreateWindowEx(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
            0, 0, rc.right, rc.bottom - BTN_H,
            hWnd, (HMENU)ID_EDIT, GetModuleHandle(NULL), NULL);

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(g_hEdit,   WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        SetWindowSubclass(g_hEdit, EditSubclassProc, 1, 0);

        SetFocus(g_hEdit);
        TriggerVoiceTyping();
        return 0;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (g_hEdit)   MoveWindow(g_hEdit,   0, 0,        w, h - BTN_H, TRUE);
        if (g_hButton) MoveWindow(g_hButton, 0, h - BTN_H, w, BTN_H,    TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_COPY)
            CopyEditToClipboard(hWnd);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_F12)
            CopyEditToClipboard(hWnd);
        return 0;

    case WM_SETFOCUS:
        if (g_hEdit)
            SetFocus(g_hEdit);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"VoiceTypingShim";
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(
        0, L"VoiceTypingShim", L"Voice Typing",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F12)
            CopyEditToClipboard(hWnd);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
