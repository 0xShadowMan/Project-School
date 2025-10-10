#include <windows.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

using namespace Gdiplus;
using namespace std;

/* MinGW link with: -lwinmm -lgdiplus */
HWND      gLogBox = NULL;
HWND      gFooter = NULL;
HBRUSH    gBgBrush = NULL;
COLORREF  gNeon = RGB(0,255,0);
ULONG_PTR gGdiplusToken = 0;
Image*    gCurrentImage = nullptr;
vector<wstring> gImages = {
    L"images\\image (1).jpg",
    L"images\\image (1).png",
    L"images\\image (2).jpg",
    L"images\\image (3).jpg",
    L"images\\image (4).jpg",
    L"images\\image (5).jpg",
    L"images\\image (6).jpg"
};
const UINT TIMER_IMAGE = 2001;

// ---- append log + save to file ----
static void AppendLog(const string &s) {
    if (!gLogBox) return;
    int len = GetWindowTextLengthA(gLogBox);
    SendMessageA(gLogBox, EM_SETSEL, len, len);
    SendMessageA(gLogBox, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
    SendMessageA(gLogBox, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
    SendMessageA(gLogBox, EM_SCROLLCARET, 0, 0);

    // Save to logs.log
    ofstream logFile("logs.log", ios::app);
    if (logFile.is_open()) {
        logFile << s << endl;
    }
}

// ---- owner-draw button ----
static void DrawOwnerButton(LPDRAWITEMSTRUCT dis, const char* text) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    BOOL pressed = (dis->itemState & ODS_SELECTED) != 0;
    BOOL focus   = (dis->itemState & ODS_FOCUS) != 0;

    COLORREF fill   = pressed ? RGB(45,45,45) : RGB(35,35,35);
    COLORREF border = pressed ? RGB(70,70,70) : RGB(90,90,90);

    HBRUSH b = CreateSolidBrush(fill);
    HPEN   pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldB = SelectObject(hdc, b);
    HGDIOBJ oldP = SelectObject(hdc, pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);

    HPEN pen2 = CreatePen(PS_INSIDEFRAME, 1, RGB(60,60,60));
    SelectObject(hdc, pen2);
    RECT r2 = rc; InflateRect(&r2, -1, -1);
    RoundRect(hdc, r2.left, r2.top, r2.right, r2.bottom, 8, 8);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0,255,0)); 
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HGDIOBJ oldF = SelectObject(hdc, hFont);
    DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (focus) {
        RECT r3 = rc; InflateRect(&r3, -3, -3);
        DrawFocusRect(hdc, &r3);
    }

    SelectObject(hdc, oldF);
    SelectObject(hdc, oldP);
    SelectObject(hdc, oldB);
    DeleteObject(b);
    DeleteObject(pen);
    DeleteObject(pen2);
}

// ---- select random image ----
static void ShowRandomImage(HWND hwnd) {
    if(gCurrentImage) delete gCurrentImage;
    int idx = rand() % gImages.size();
    gCurrentImage = new Image(gImages[idx].c_str());
    InvalidateRect(hwnd, NULL, TRUE);
}

// ---- run command and capture output ----
string RunCommand(const string &cmd) {
    string result;
    char buffer[256];
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "Error: Unable to run command.";
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    pclose(pipe);
    return result;
}

// ---- window proc ----
LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_CREATE:
        {
            srand((unsigned int)time(NULL));

            CreateWindowA("STATIC", "=== Welcome in ShadowTools ===\r\nCreated by Shohan",
                WS_CHILD|WS_VISIBLE|SS_CENTER, 10, 10, 450, 40, hwnd, (HMENU)100, GetModuleHandle(NULL), NULL);

            CreateWindowA("BUTTON", "CopyMan", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                30, 60, 130, 40, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
            CreateWindowA("BUTTON", "Hack WIFI", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                175, 60, 130, 40, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
            CreateWindowA("BUTTON", "Exit", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                320, 60, 130, 40, hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);

            // ---- full output box ----
            gLogBox = CreateWindowExA(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                470, 10, 400, 400,  
                hwnd, (HMENU)200, GetModuleHandle(NULL), NULL
            );
            HFONT hFont = CreateFontA(
                16, 0, 0,0, FW_NORMAL, FALSE,FALSE,FALSE, 
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FF_DONTCARE, "Consolas"
            );
            SendMessage(gLogBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            AppendLog("[INFO] Everything is easy for Shohan :)");
            AppendLog("-------------------------------------------");

            gFooter = CreateWindowA("STATIC", "ShadowTools - Hacking made easy!",
                WS_CHILD|WS_VISIBLE|SS_CENTER, 10, 420, 660, 20, hwnd, (HMENU)300, GetModuleHandle(NULL), NULL);

            ShowRandomImage(hwnd);
            SetTimer(hwnd, TIMER_IMAGE, 5000, NULL); 
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            if((HWND)lParam==gFooter) SetTextColor(hdc, RGB(210,210,210)); 
            else SetTextColor(hdc, gNeon);
            SetBkColor(hdc, RGB(0,0,0));
            return (LRESULT)gBgBrush;
        }
    case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, gNeon);
            SetBkColor(hdc, RGB(0,0,0));
            return (LRESULT)gBgBrush;
        }

    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if(dis->CtlID==1) DrawOwnerButton(dis,"CopyMan");
            else if(dis->CtlID==2) DrawOwnerButton(dis,"Hack WIFI");
            else if(dis->CtlID==3) DrawOwnerButton(dis,"Exit");
            return TRUE;
        }

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case 1: AppendLog("[INFO] Checking ... (mockup)"); AppendLog("[DONE] Looks good!"); break;

            case 2:
            {
                AppendLog("[INFO] Starting Wi-Fi hacking...");
                AppendLog("[+] Scanning all WIFI logs...");
                Sleep(700);

                AppendLog("[+] Capturing handshake...");
                Sleep(1000);
                AppendLog("[+] Handshake captured successfully!");
                Sleep(500);
        
                AppendLog("[+] Starting Fetch attack...");


                // Fetch real Wi-Fi profiles and passwords
                const char* cmd =
                    "powershell -Command \""
                    "$profiles = netsh wlan show profiles | Select-String 'All User Profile' | ForEach-Object { ($_ -split ':')[1].Trim() }; "
                    "if ($profiles.Count -eq 0) { Write-Host 'No Wi-Fi profiles found on this PC.' } "
                    "else { "
                    "$results = foreach ($profile in $profiles) { "
                    "$details = netsh wlan show profile name=$profile key=clear; "
                    "$keyContent = ($details | Select-String 'Key Content') -replace '.*:\\\\s*',''; "
                    "[PSCustomObject]@{ 'Wi-Fi Name' = $profile; 'Password' = if ($keyContent) { $keyContent } else { 'No Password / Open Network' } } "
                    "}; $results }\"";

                FILE* pipe = popen(cmd, "r");
                if (!pipe) {
                    AppendLog("[ERROR] Failed to run PowerShell command.");
                    break;
                }

                char buffer[256];
                vector<string> passwords;
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    string line = buffer;
                    if (!line.empty() && line.back() == '\n') line.pop_back();
                    // Skip headers/empty lines
                    if (line.find("Wi-Fi Name") != string::npos || line.find("Password") != string::npos || line.find("---") != string::npos)
                        continue;
                    if (!line.empty()) passwords.push_back(line);
                }
                pclose(pipe);

                for (const auto& pwd : passwords) {
                    AppendLog("[+] Trying password: " + pwd);
                    Sleep(800);
                }

                if (!passwords.empty()) {
                    AppendLog("[+] Password found: " + passwords.back());
                    AppendLog("[SUCCESS] Wi-Fi hacking completed!");
                    AppendLog("-------------------------------------------");

                } else {
                    AppendLog("[!] No Wi-Fi passwords found.");
                    AppendLog("-------------------------------------------");
                }
            }
            break;
                
            case 3: PostQuitMessage(0); break;
        }
        return 0;

    case WM_TIMER:
        if(wParam==TIMER_IMAGE) ShowRandomImage(hwnd);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd,&ps);
            RECT rc; GetClientRect(hwnd,&rc);
            FillRect(hdc,&rc,gBgBrush);

            if(gCurrentImage){
                Graphics g(hdc);
                int bannerWidth = 450; 
                int bannerHeight = gCurrentImage->GetHeight() * bannerWidth / gCurrentImage->GetWidth();
                Rect r(10,110,bannerWidth,bannerHeight);
                g.DrawImage(gCurrentImage,r);
                Pen pen(Color(255,200,0,0),2.0f);
                g.DrawRectangle(&pen,r);
            }

            RECT client; GetClientRect(hwnd,&client);
            SetWindowPos(gFooter,NULL,10,client.bottom-30,client.right-20,20,SWP_NOZORDER|SWP_NOACTIVATE);

            EndPaint(hwnd,&ps);
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd,TIMER_IMAGE);
        if(gCurrentImage) delete gCurrentImage;
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        InvalidateRect(hwnd,NULL,TRUE);
        return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

// ---- main ----
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR,int){
    GdiplusStartupInput gpsi;
    GdiplusStartup(&gGdiplusToken,&gpsi,NULL);

    gBgBrush = CreateSolidBrush(RGB(0,0,0));

    mciSendStringA("open \"music.mp3\" type mpegvideo alias mp3",NULL,0,0);
    mciSendStringA("setaudio mp3 volume to 400",NULL,0,0);
    mciSendStringA("play mp3 repeat",NULL,0,0);

    const char* CLASS_NAME = "ShadowToolsMainClass";
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = MainProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = gBgBrush;
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, "ShadowTools",
        WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
        120,120,900,470,NULL,NULL,hInst,NULL);
    ShowWindow(hwnd,SW_SHOW);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)>0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    mciSendStringA("close mp3",NULL,0,0);
    if(gCurrentImage) delete gCurrentImage;
    if(gGdiplusToken) GdiplusShutdown(gGdiplusToken);
    if(gBgBrush) DeleteObject(gBgBrush);
    return 0;
}
