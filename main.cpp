#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <cwctype>
#include "proper_nouns.h"

static HWND g_hEdit   = NULL;
static HWND g_hCopy   = NULL;
static HWND g_hFixup  = NULL;

#define ID_EDIT              1
#define ID_COPY              2
#define ID_FIXUP             3
#define BTN_H                40
#define TIMER_UNFLASH_COPY   1
#define TIMER_UNFLASH_FIXUP  2

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

static void FlashButton(HWND hWnd, HWND hBtn, UINT_PTR timerId)
{
    SendMessage(hBtn, BM_SETSTATE, TRUE, 0);
    SetTimer(hWnd, timerId, 150, NULL);
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

static bool IsProperNoun(const WCHAR *word)
{
    int lo = 0, hi = kProperNounsCount - 1;
    while (lo <= hi)
    {
        int mid = (lo + hi) / 2;
        int cmp = wcscmp(kProperNouns[mid], word);
        if (cmp == 0) return true;
        if (cmp < 0)  lo = mid + 1;
        else          hi = mid - 1;
    }
    return false;
}

static void FixupText()
{
    int len = GetWindowTextLength(g_hEdit);
    if (len == 0)
        return;

    WCHAR *buf = new WCHAR[len + 1];
    GetWindowText(g_hEdit, buf, len + 1);

    // Lower-case any word that starts with a capital letter but is not at the
    // beginning of a sentence and is not an all-caps abbreviation (e.g. NASA).
    bool sentenceStart = true;

    for (int i = 0; i < len; )
    {
        if (iswalpha(buf[i]))
        {
            int start = i;
            while (i < len && iswalpha(buf[i]))
                i++;

            if (!sentenceStart && iswupper(buf[start]))
            {
                // Leave all-caps abbreviations alone
                bool allCaps = (i - start > 1);
                for (int j = start; j < i && allCaps; j++)
                    if (!iswupper(buf[j]))
                        allCaps = false;

                if (!allCaps)
                {
                    // Temporarily null-terminate to look up in the dictionary
                    WCHAR saved = buf[i];
                    buf[i] = L'\0';
                    bool proper = IsProperNoun(buf + start);
                    buf[i] = saved;

                    if (!proper)
                        buf[start] = (WCHAR)towlower(buf[start]);
                }
            }
            sentenceStart = false;
        }
        else
        {
            WCHAR c = buf[i];
            if (c == L'.' || c == L'!' || c == L'?')
                sentenceStart = true;
            // spaces and other punctuation do not change sentenceStart
            i++;
        }
    }

    SetWindowText(g_hEdit, buf);
    delete[] buf;
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
        int half = rc.right / 2;

        g_hFixup = CreateWindowEx(
            0, L"BUTTON", L"Fix up (F11)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, rc.bottom - BTN_H, half, BTN_H,
            hWnd, (HMENU)ID_FIXUP, GetModuleHandle(NULL), NULL);

        g_hCopy = CreateWindowEx(
            0, L"BUTTON", L"Copy (F12)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            half, rc.bottom - BTN_H, rc.right - half, BTN_H,
            hWnd, (HMENU)ID_COPY, GetModuleHandle(NULL), NULL);

        g_hEdit = CreateWindowEx(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
            0, 0, rc.right, rc.bottom - BTN_H,
            hWnd, (HMENU)ID_EDIT, GetModuleHandle(NULL), NULL);

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(g_hEdit,  WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_hCopy,  WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_hFixup, WM_SETFONT, (WPARAM)hFont, TRUE);

        SetWindowSubclass(g_hEdit, EditSubclassProc, 1, 0);

        SetFocus(g_hEdit);
        TriggerVoiceTyping();
        return 0;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        int half = w / 2;
        if (g_hEdit)  MoveWindow(g_hEdit,  0,    0,        w,          h - BTN_H, TRUE);
        if (g_hFixup) MoveWindow(g_hFixup, 0,    h - BTN_H, half,      BTN_H,     TRUE);
        if (g_hCopy)  MoveWindow(g_hCopy,  half, h - BTN_H, w - half,  BTN_H,     TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_COPY)
            CopyEditToClipboard(hWnd);
        else if (LOWORD(wParam) == ID_FIXUP)
            FixupText();
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_F12)
        {
            CopyEditToClipboard(hWnd);
            FlashButton(hWnd, g_hCopy, TIMER_UNFLASH_COPY);
        }
        else if (wParam == VK_F11)
        {
            FixupText();
            FlashButton(hWnd, g_hFixup, TIMER_UNFLASH_FIXUP);
        }
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_UNFLASH_COPY)
        {
            KillTimer(hWnd, TIMER_UNFLASH_COPY);
            SendMessage(g_hCopy, BM_SETSTATE, FALSE, 0);
        }
        else if (wParam == TIMER_UNFLASH_FIXUP)
        {
            KillTimer(hWnd, TIMER_UNFLASH_FIXUP);
            SendMessage(g_hFixup, BM_SETSTATE, FALSE, 0);
        }
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
        if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam == VK_F12)
            {
                CopyEditToClipboard(hWnd);
                FlashButton(hWnd, g_hCopy, TIMER_UNFLASH_COPY);
            }
            else if (msg.wParam == VK_F11)
            {
                FixupText();
                FlashButton(hWnd, g_hFixup, TIMER_UNFLASH_FIXUP);
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
