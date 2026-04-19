#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <commdlg.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include "export.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace Gdiplus;

// UI Control IDs
#define ID_BTN_FRONT    101
#define ID_BTN_BACK     102
#define ID_CBO_PAPER    103
#define ID_TXT_COPIES   104
#define ID_CHK_DUPLEX   105
#define ID_BTN_PRINT    106
#define ID_CBO_EDGE     109
#define ID_CBO_ORIENT   110

#define ID_BTN_PREV_PG1 301
#define ID_BTN_PREV_PG2 302

#define ID_MENU_IMPORT  701
#define ID_MENU_EXPORT  702
#define ID_MENU_ABOUT   703

#define ID_MENU_THEME_LIGHT 801
#define ID_MENU_THEME_DARK  802
#define ID_MENU_THEME_AZURE 803

#define IDI_APP_ICON    1

#define ID_TXT_ML       503
#define ID_TXT_MR       504
#define ID_TXT_MT       505
#define ID_TXT_MB       506

// Editor Dialog IDs
#define ID_ED_OK      401
#define ID_ED_CANCEL  402
#define ID_ED_ROT     403
#define ID_ED_SCALE   404
#define ID_ED_X       405
#define ID_ED_Y       406

struct ImageTransform {
    float rotate = 0.0f;
    float scale = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
};

struct EditorData {
    Image* img;
    ImageTransform xform;
    bool accepted;
};

struct PaperInfo {
    const wchar_t* name;
    float w;
    float h;
};

const PaperInfo PAPER_LIST[] = {
    { L"Letter (8.5\" x 11\")", 215.9f, 279.4f },
    { L"Legal (8.5\" x 14\")", 215.9f, 355.6f },
    { L"Executive (7.25\" x 10.5\")", 184.15f, 266.7f },
    { L"A4 (210 mm x 297 mm)", 210.0f, 297.0f },
    { L"A3 (297 mm x 420 mm)", 297.0f, 420.0f },
    { L"A5 (148 mm x 210 mm)", 148.0f, 210.0f },
    { L"B4 (250 mm x 353 mm)", 250.0f, 353.0f },
    { L"B5 (176 mm x 250 mm)", 176.0f, 250.0f },
    { L"Folio (8.5\" x 13\")", 215.9f, 330.2f },
    { L"Quarto (215 mm x 275 mm)", 215.0f, 275.0f },
    { L"Statement (5.5\" x 8.5\")", 139.7f, 215.9f },
    { L"Tabloid (11\" x 17\")", 279.4f, 431.8f },
    { L"Ledger (17\" x 11\")", 431.8f, 279.4f },
    { L"Envelope #10 (4.125\" x 9.5\")", 104.775f, 241.3f },
    { L"Envelope DL (110 mm x 220 mm)", 110.0f, 220.0f },
    { L"Envelope C5 (162 mm x 229 mm)", 162.0f, 229.0f },
    { L"Envelope B5 (176 mm x 250 mm)", 176.0f, 250.0f },
    { L"Envelope Monarch (3.875\" x 7.5\")", 98.425f, 190.5f },
    { L"Envelope Personal (3.625\" x 6.5\")", 92.075f, 165.1f },
    { L"Custom", 0.0f, 0.0f }
};
const int NUM_PAPERS = sizeof(PAPER_LIST) / sizeof(PaperInfo);

struct ThemeColors {
    COLORREF bg;
    COLORREF text;
    COLORREF controlBg;
    COLORREF border;
    HBRUSH hBrushBg;
    HBRUSH hBrushControl;
};

ThemeColors g_themes[] = {
    { RGB(249, 249, 249), RGB(30, 30, 30), RGB(255, 255, 255), RGB(210, 210, 210), NULL, NULL },     // Light
    { RGB(25, 25, 25), RGB(240, 240, 240), RGB(40, 40, 40), RGB(60, 60, 60), NULL, NULL },           // Dark
    { RGB(235, 245, 255), RGB(0, 60, 120), RGB(250, 253, 255), RGB(180, 210, 240), NULL, NULL }    // Azure
};
int g_currentTheme = 0;

HWND hFrontLabel, hBackLabel, hPaperCombo, hCopiesEdit, hDuplexCheck, hEdgeCombo, hOrientCombo;
HWND hML, hMR, hMT, hMB;
HWND hPreviewArea, hBtnPage1, hBtnPage2, hGroupSettings;

PrintSettings g_settings;
std::wstring g_frontPathW, g_backPathW;
ImageTransform g_frontXform, g_backXform;
Image* g_imgFront = nullptr;
Image* g_imgBack = nullptr;
int g_previewPage = 0;

void ApplyTheme(HWND hwnd) {
    ThemeColors& t = g_themes[g_currentTheme];
    if (t.hBrushBg) DeleteObject(t.hBrushBg);
    if (t.hBrushControl) DeleteObject(t.hBrushControl);
    t.hBrushBg = CreateSolidBrush(t.bg);
    t.hBrushControl = CreateSolidBrush(t.controlBg);

    // Dark Mode title bar support (Windows 10 17763+)
    BOOL dark = (g_currentTheme == 1);
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE

    InvalidateRect(hwnd, NULL, TRUE);
    EnumChildWindows(hwnd, [](HWND c, LPARAM p){ InvalidateRect(c, NULL, TRUE); return TRUE; }, 0);
}

void UpdateUIFromSettings() {
    wchar_t buf[32];
    if (hOrientCombo) SendMessage(hOrientCombo, CB_SETCURSEL, g_settings.isLandscape ? 1 : 0, 0);
    int pIdx = 0;
    for (int i=0; i<NUM_PAPERS-1; ++i) {
        if (abs(PAPER_LIST[i].w - g_settings.paperWidthMm) < 0.1f && abs(PAPER_LIST[i].h - g_settings.paperHeightMm) < 0.1f) { pIdx = i; break; }
    }
    if (hPaperCombo) SendMessage(hPaperCombo, CB_SETCURSEL, pIdx, 0);
    if (hDuplexCheck) SendMessage(hDuplexCheck, BM_SETCHECK, g_settings.duplex ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hEdgeCombo) SendMessage(hEdgeCombo, CB_SETCURSEL, g_settings.flipEdge, 0);
    swprintf(buf, 32, L"%.1f", g_settings.marginLeft); if (hML) SetWindowTextW(hML, buf);
    swprintf(buf, 32, L"%.1f", g_settings.marginRight); if (hMR) SetWindowTextW(hMR, buf);
    swprintf(buf, 32, L"%.1f", g_settings.marginTop); if (hMT) SetWindowTextW(hMT, buf);
    swprintf(buf, 32, L"%.1f", g_settings.marginBottom); if (hMB) SetWindowTextW(hMB, buf);
    swprintf(buf, 32, L"%d", g_settings.copies); if (hCopiesEdit) SetWindowTextW(hCopiesEdit, buf);
    if (hFrontLabel) SetWindowTextW(hFrontLabel, g_frontPathW.empty() ? L"No Image Selected" : g_frontPathW.c_str());
    if (hBackLabel) SetWindowTextW(hBackLabel, g_backPathW.empty() ? L"No Image Selected" : g_backPathW.c_str());
}

void ReadSettingsFromUI() {
    wchar_t buf[32];
    if (hCopiesEdit) { GetWindowTextW(hCopiesEdit, buf, 32); g_settings.copies = _wtoi(buf); if (g_settings.copies <= 0) g_settings.copies = 1; }
    if (hPaperCombo) {
        int idx = (int)SendMessage(hPaperCombo, CB_GETCURSEL, 0, 0);
        if (idx >= 0 && idx < NUM_PAPERS - 1) { g_settings.paperWidthMm = PAPER_LIST[idx].w; g_settings.paperHeightMm = PAPER_LIST[idx].h; }
    }
    if (hOrientCombo) g_settings.isLandscape = (SendMessage(hOrientCombo, CB_GETCURSEL, 0, 0) == 1);
    if (hDuplexCheck) g_settings.duplex = (SendMessage(hDuplexCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (hEdgeCombo) g_settings.flipEdge = (int)SendMessage(hEdgeCombo, CB_GETCURSEL, 0, 0);
    if (hML) { GetWindowTextW(hML, buf, 32); g_settings.marginLeft = (float)_wtof(buf); }
    if (hMR) { GetWindowTextW(hMR, buf, 32); g_settings.marginRight = (float)_wtof(buf); }
    if (hMT) { GetWindowTextW(hMT, buf, 32); g_settings.marginTop = (float)_wtof(buf); }
    if (hMB) { GetWindowTextW(hMB, buf, 32); g_settings.marginBottom = (float)_wtof(buf); }
    g_settings.cardWidthMm = 85.6f; g_settings.cardHeightMm = 54.0f;
}

void SaveAppSettings() {
    ReadSettingsFromUI();
    std::wofstream f(L"settings.ini");
    if (f.is_open()) {
        f << g_settings.paperWidthMm << L"\n" << g_settings.paperHeightMm << L"\n" << g_settings.marginLeft << L"\n" << g_settings.marginRight << L"\n";
        f << g_settings.marginTop << L"\n" << g_settings.marginBottom << L"\n" << (g_settings.isLandscape ? 1 : 0) << L"\n" << (g_settings.duplex ? 1 : 0) << L"\n" << g_settings.flipEdge << L"\n";
        f << g_currentTheme << L"\n";
        f.close();
    }
}

void LoadAppSettings() {
    std::wifstream f(L"settings.ini");
    if (f.is_open()) {
        int iLand, iDup;
        f >> g_settings.paperWidthMm >> g_settings.paperHeightMm >> g_settings.marginLeft >> g_settings.marginRight >> g_settings.marginTop >> g_settings.marginBottom >> iLand >> iDup >> g_settings.flipEdge >> g_currentTheme;
        g_settings.isLandscape = (iLand != 0); g_settings.duplex = (iDup != 0);
        f.close();
    } else {
        g_settings.paperWidthMm = 210.0f; g_settings.paperHeightMm = 297.0f; g_settings.cardWidthMm = 85.6f; g_settings.cardHeightMm = 54.0f;
        g_settings.marginLeft = 10.0f; g_settings.marginRight = 10.0f; g_settings.marginTop = 10.0f; g_settings.marginBottom = 10.0f;
        g_settings.copies = 1; g_settings.isLandscape = false; g_settings.duplex = true; g_settings.flipEdge = 0; g_currentTheme = 0;
    }
}

void ExportWorkspace(HWND hwnd) {
    ReadSettingsFromUI();
    OPENFILENAMEW ofn = {0}; wchar_t szFile[260] = { 0 }; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = 260; ofn.lpstrFilter = L"ID Card Workspace (*.idw)\0*.idw\0"; ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) {
        std::wofstream f(szFile);
        if (f.is_open()) {
            f << g_frontPathW << L"|" << g_frontXform.rotate << L"|" << g_frontXform.scale << L"|" << g_frontXform.panX << L"|" << g_frontXform.panY << L"\n";
            f << g_backPathW << L"|" << g_backXform.rotate << L"|" << g_backXform.scale << L"|" << g_backXform.panX << L"|" << g_backXform.panY << L"\n";
            f << g_settings.paperWidthMm << L"|" << g_settings.paperHeightMm << L"|" << g_settings.marginLeft << L"|" << g_settings.marginRight << L"|" << g_settings.marginTop << L"|" << g_settings.marginBottom << L"|" << (g_settings.isLandscape ? 1 : 0) << L"|" << (g_settings.duplex ? 1 : 0) << L"|" << g_settings.flipEdge << L"\n";
            f.close();
        }
    }
}

void ImportWorkspace(HWND hwnd) {
    OPENFILENAMEW ofn = {0}; wchar_t szFile[260] = { 0 }; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = 260; ofn.lpstrFilter = L"ID Card Workspace (*.idw)\0*.idw\0"; ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        std::wifstream f(ofn.lpstrFile);
        if (f.is_open()) {
            std::wstring line;
            auto parse = [&](std::wstring& l, std::wstring& p, ImageTransform& x) {
                size_t pos = l.find(L"|"); p = l.substr(0, pos); l.erase(0, pos+1);
                pos = l.find(L"|"); x.rotate = (float)_wtof(l.substr(0, pos).c_str()); l.erase(0, pos+1);
                pos = l.find(L"|"); x.scale = (float)_wtof(l.substr(0, pos).c_str()); l.erase(0, pos+1);
                pos = l.find(L"|"); x.panX = (float)_wtof(l.substr(0, pos).c_str()); l.erase(0, pos+1);
                x.panY = (float)_wtof(l.c_str());
            };
            std::getline(f, line); parse(line, g_frontPathW, g_frontXform);
            std::getline(f, line); parse(line, g_backPathW, g_backXform);
            int iLand, iDup;
            f >> g_settings.paperWidthMm; f.ignore(1); f >> g_settings.paperHeightMm; f.ignore(1); f >> g_settings.marginLeft; f.ignore(1); f >> g_settings.marginRight; f.ignore(1); f >> g_settings.marginTop; f.ignore(1); f >> g_settings.marginBottom; f.ignore(1); f >> iLand; f.ignore(1); f >> iDup; f.ignore(1); f >> g_settings.flipEdge;
            g_settings.isLandscape = (iLand != 0); g_settings.duplex = (iDup != 0);
            f.close();
            if (g_imgFront) delete g_imgFront; g_imgFront = g_frontPathW.empty() ? nullptr : new Image(g_frontPathW.c_str());
            if (g_imgBack) delete g_imgBack; g_imgBack = g_backPathW.empty() ? nullptr : new Image(g_backPathW.c_str());
            UpdateUIFromSettings(); InvalidateRect(hPreviewArea, NULL, TRUE);
        }
    }
}

void DrawIDCard(Graphics& g, Image* img, const ImageTransform& xform, float x, float y, float w, float h, float cardW_mm) {
    if (!img) { Pen pen(Color(200, 200, 200), 1.0f); pen.SetDashStyle(DashStyleDash); g.DrawRectangle(&pen, x, y, w, h); return; }
    GraphicsState state = g.Save(); RectF cardRect(x, y, w, h); g.SetClip(cardRect);
    float pixelsPerMm = w / cardW_mm, centerX = x + w / 2.0f, centerY = y + h / 2.0f, imgW = (float)img->GetWidth(), imgH = (float)img->GetHeight(), baseScale = std::min(w / imgW, h / imgH);
    Matrix matrix; matrix.Translate(centerX + xform.panX * pixelsPerMm, centerY + xform.panY * pixelsPerMm); matrix.Rotate(xform.rotate); matrix.Scale(baseScale * xform.scale, baseScale * xform.scale); matrix.Translate(-imgW / 2.0f, -imgH / 2.0f);
    g.SetTransform(&matrix); g.DrawImage(img, 0.0f, 0.0f, imgW, imgH); g.Restore(state);
    Pen borderPen(Color(100, 100, 100), 1.0f); g.DrawRectangle(&borderPen, cardRect);
}

LRESULT CALLBACK EditorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorData* data = (EditorData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam; data = (EditorData*)pcs->lpCreateParams; SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        int x = 20, y = 380;
        CreateWindowW(L"STATIC", L"Rotate:", WS_VISIBLE | WS_CHILD, x, y, 60, 20, hwnd, NULL, NULL, NULL);
        HWND hRot = CreateWindowW(TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x+70, y, 220, 30, hwnd, (HMENU)ID_ED_ROT, NULL, NULL);
        SendMessage(hRot, TBM_SETRANGE, TRUE, MAKELONG(0, 360)); SendMessage(hRot, TBM_SETPOS, TRUE, (int)data->xform.rotate);
        y += 40;
        CreateWindowW(L"STATIC", L"Zoom:", WS_VISIBLE | WS_CHILD, x, y, 60, 20, hwnd, NULL, NULL, NULL);
        HWND hScale = CreateWindowW(TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x+70, y, 220, 30, hwnd, (HMENU)ID_ED_SCALE, NULL, NULL);
        SendMessage(hScale, TBM_SETRANGE, TRUE, MAKELONG(10, 500)); SendMessage(hScale, TBM_SETPOS, TRUE, (int)(data->xform.scale * 100));
        y += 40;
        CreateWindowW(L"STATIC", L"Pan X/Y:", WS_VISIBLE | WS_CHILD, x, y, 60, 20, hwnd, NULL, NULL, NULL);
        HWND hX = CreateWindowW(TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD, x+70, y, 105, 30, hwnd, (HMENU)ID_ED_X, NULL, NULL);
        HWND hY = CreateWindowW(TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD, x+175, y, 105, 30, hwnd, (HMENU)ID_ED_Y, NULL, NULL);
        SendMessage(hX, TBM_SETRANGE, TRUE, MAKELONG(-100, 100)); SendMessage(hY, TBM_SETRANGE, TRUE, MAKELONG(-100, 100));
        SendMessage(hX, TBM_SETPOS, TRUE, (int)data->xform.panX); SendMessage(hY, TBM_SETPOS, TRUE, (int)data->xform.panY);
        y += 50;
        CreateWindowW(L"BUTTON", L"SAVE CHANGES", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, 140, 40, hwnd, (HMENU)ID_ED_OK, NULL, NULL);
        CreateWindowW(L"BUTTON", L"CANCEL", WS_VISIBLE | WS_CHILD, x+150, y, 130, 40, hwnd, (HMENU)ID_ED_CANCEL, NULL, NULL);
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT); EnumChildWindows(hwnd, [](HWND c, LPARAM f){ SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
        break;
    }
    case WM_HSCROLL: {
        data->xform.rotate = (float)SendMessage(GetDlgItem(hwnd, ID_ED_ROT), TBM_GETPOS, 0, 0);
        data->xform.scale = (float)SendMessage(GetDlgItem(hwnd, ID_ED_SCALE), TBM_GETPOS, 0, 0) / 100.0f;
        data->xform.panX = (float)SendMessage(GetDlgItem(hwnd, ID_ED_X), TBM_GETPOS, 0, 0);
        data->xform.panY = (float)SendMessage(GetDlgItem(hwnd, ID_ED_Y), TBM_GETPOS, 0, 0);
        InvalidateRect(hwnd, NULL, TRUE); break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); Graphics g(hdc); g.Clear(Color(255, 255, 255));
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        DrawIDCard(g, data->img, data->xform, 40, 40, 85.6f * 3.0f, 54.0f * 3.0f, 85.6f);
        EndPaint(hwnd, &ps); break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_ED_OK) { data->accepted = true; DestroyWindow(hwnd); }
        if (LOWORD(wParam) == ID_ED_CANCEL) { data->accepted = false; DestroyWindow(hwnd); }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool OpenEditor(HWND parent, Image* img, ImageTransform& xform) {
    EditorData data = { img, xform, false };
    WNDCLASSW wc = {0}; wc.lpfnWndProc = EditorProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = L"EditorClass"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc);
    HWND dlg = CreateWindowW(L"EditorClass", L"Adjust Image", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 360, 600, parent, NULL, wc.hInstance, &data);
    EnableWindow(parent, FALSE); ShowWindow(dlg, SW_SHOW);
    MSG msg; while (IsWindow(dlg) && GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    EnableWindow(parent, TRUE); SetForegroundWindow(parent);
    return data.accepted;
}

void HandleFileSelection(HWND hwnd, int side, std::wstring path) {
    if (path.empty()) {
        OPENFILENAMEW ofn = {0}; wchar_t szFile[260] = { 0 }; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = 260; ofn.lpstrFilter = L"Images\0*.jpg;*.jpeg;*.png;*.bmp\0All\0*.*\0"; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) path = szFile;
    }
    if (!path.empty()) {
        Image* img = new Image(path.c_str());
        if (img->GetLastStatus() == Ok) {
            ImageTransform& xform = (side == 1 ? g_frontXform : g_backXform);
            ImageTransform temp = xform;
            if (OpenEditor(hwnd, img, temp)) {
                if (side == 1) { if (g_imgFront) delete g_imgFront; g_imgFront = img; g_frontXform = temp; g_frontPathW = path; SetWindowTextW(hFrontLabel, path.c_str()); }
                else { if (g_imgBack) delete g_imgBack; g_imgBack = img; g_backXform = temp; g_backPathW = path; SetWindowTextW(hBackLabel, path.c_str()); }
                InvalidateRect(hPreviewArea, NULL, TRUE);
            } else delete img;
        } else delete img;
    }
}

LRESULT CALLBACK PreviewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc);
        Bitmap bmp(rc.right, rc.bottom); Graphics g(&bmp);
        ThemeColors& t = g_themes[g_currentTheme];
        g.Clear(Color(GetRValue(t.bg), GetGValue(t.bg), GetBValue(t.bg)));
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        ReadSettingsFromUI();
        float pW = g_settings.isLandscape ? g_settings.paperHeightMm : g_settings.paperWidthMm, pH = g_settings.isLandscape ? g_settings.paperWidthMm : g_settings.paperHeightMm;
        if (pW <= 0) pW = 1; if (pH <= 0) pH = 1;
        float scale = std::min((float)rc.right / pW, (float)rc.bottom / pH) * 0.92f, offX = (rc.right - pW * scale) / 2.0f, offY = (rc.bottom - pH * scale) / 2.0f;
        g.FillRectangle(&SolidBrush(Color(255, 255, 255)), offX, offY, pW * scale, pH * scale);
        g.DrawRectangle(&Pen(Color(180, 180, 180)), offX, offY, pW * scale, pH * scale);
        Pen marginPen(Color(100, 200, 200, 255), 1.0f); marginPen.SetDashStyle(DashStyleDash);
        g.DrawRectangle(&marginPen, offX + g_settings.marginLeft * scale, offY + g_settings.marginTop * scale, (pW - g_settings.marginLeft - g_settings.marginRight) * scale, (pH - g_settings.marginTop - g_settings.marginBottom) * scale);
        auto positions = CalculateLayout(g_settings);
        for (const auto& pos : positions) {
            if (pos.page != g_previewPage) continue;
            DrawIDCard(g, pos.isFront ? g_imgFront : g_imgBack, pos.isFront ? g_frontXform : g_backXform, offX + pos.x_mm * scale, offY + pos.y_mm * scale, g_settings.cardWidthMm * scale, g_settings.cardHeightMm * scale, g_settings.cardWidthMm);
        }
        Graphics screen(hdc); screen.DrawImage(&bmp, 0, 0); EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        DragAcceptFiles(hwnd, TRUE); LoadAppSettings();
        HMENU hMenuBar = CreateMenu(), hFileMenu = CreateMenu(), hThemeMenu = CreateMenu();
        AppendMenuW(hFileMenu, MF_STRING, ID_MENU_IMPORT, L"Import Workspace...");
        AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXPORT, L"Export Workspace...");
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
        AppendMenuW(hThemeMenu, MF_STRING, ID_MENU_THEME_LIGHT, L"Light Theme");
        AppendMenuW(hThemeMenu, MF_STRING, ID_MENU_THEME_DARK, L"Dark Theme");
        AppendMenuW(hThemeMenu, MF_STRING, ID_MENU_THEME_AZURE, L"Azure Theme");
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hThemeMenu, L"Theme");
        AppendMenuW(hMenuBar, MF_STRING, ID_MENU_ABOUT, L"About");
        SetMenu(hwnd, hMenuBar);
        
        int y = 20, x = 20;
        hGroupSettings = CreateWindowW(L"BUTTON", L"PRINT SETTINGS", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 5, 200, 780, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"FRONT SIDE", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, 180, 80, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"SELECT IMAGE", WS_VISIBLE | WS_CHILD, x+5, y+20, 170, 30, hwnd, (HMENU)ID_BTN_FRONT, NULL, NULL);
        hFrontLabel = CreateWindowW(L"STATIC", L"No Image", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, x+5, y+55, 170, 20, hwnd, NULL, NULL, NULL);
        y += 95;
        CreateWindowW(L"BUTTON", L"BACK SIDE", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, 180, 80, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"SELECT IMAGE", WS_VISIBLE | WS_CHILD, x+5, y+20, 170, 30, hwnd, (HMENU)ID_BTN_BACK, NULL, NULL);
        hBackLabel = CreateWindowW(L"STATIC", L"No Image", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, x+5, y+55, 170, 20, hwnd, NULL, NULL, NULL);
        y += 100;
        CreateWindowW(L"STATIC", L"Orientation:", WS_VISIBLE | WS_CHILD, x, y, 180, 20, hwnd, NULL, NULL, NULL);
        hOrientCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, x, y+20, 180, 100, hwnd, (HMENU)ID_CBO_ORIENT, NULL, NULL);
        SendMessageW(hOrientCombo, CB_ADDSTRING, 0, (LPARAM)L"Portrait"); SendMessageW(hOrientCombo, CB_ADDSTRING, 0, (LPARAM)L"Landscape");
        y += 55;
        CreateWindowW(L"STATIC", L"Paper Size:", WS_VISIBLE | WS_CHILD, x, y, 180, 20, hwnd, NULL, NULL, NULL);
        hPaperCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, x, y+20, 180, 400, hwnd, (HMENU)ID_CBO_PAPER, NULL, NULL);
        for (int i=0; i<NUM_PAPERS; ++i) SendMessageW(hPaperCombo, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)PAPER_LIST[i].name);
        y += 60;
        CreateWindowW(L"STATIC", L"Margins (L/R/T/B) mm:", WS_VISIBLE | WS_CHILD, x, y, 180, 20, hwnd, NULL, NULL, NULL);
        hML = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, x, y+20, 40, 22, hwnd, NULL, NULL, NULL);
        hMR = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, x+45, y+20, 40, 22, hwnd, NULL, NULL, NULL);
        hMT = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, x+90, y+20, 40, 22, hwnd, NULL, NULL, NULL);
        hMB = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, x+135, y+20, 40, 22, hwnd, NULL, NULL, NULL);
        y += 55;
        hDuplexCheck = CreateWindowW(L"BUTTON", L"Double Sided", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, 180, 20, hwnd, (HMENU)ID_CHK_DUPLEX, NULL, NULL);
        y += 25;
        hEdgeCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, x, y, 180, 100, hwnd, (HMENU)ID_CBO_EDGE, NULL, NULL);
        SendMessageW(hEdgeCombo, CB_ADDSTRING, 0, (LPARAM)L"Flip on Long Edge"); SendMessageW(hEdgeCombo, CB_ADDSTRING, 0, (LPARAM)L"Flip on Short Edge");
        y += 40;
        CreateWindowW(L"STATIC", L"Copies:", WS_VISIBLE | WS_CHILD, x, y, 60, 20, hwnd, NULL, NULL, NULL);
        hCopiesEdit = CreateWindowW(L"EDIT", L"1", WS_VISIBLE | WS_CHILD | WS_BORDER, x+60, y, 40, 22, hwnd, NULL, NULL, NULL);
        y += 60;
        CreateWindowW(L"BUTTON", L"PRINT NOW", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, 180, 60, hwnd, (HMENU)ID_BTN_PRINT, NULL, NULL);
        
        WNDCLASSW pwc = {0}; pwc.lpfnWndProc = PreviewProc; pwc.hInstance = GetModuleHandle(NULL); pwc.lpszClassName = L"PreviewClass"; pwc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&pwc);
        hPreviewArea = CreateWindowW(L"PreviewClass", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 10, 420, 720, hwnd, NULL, NULL, NULL);
        hBtnPage1 = CreateWindowW(L"BUTTON", L"Page 1", WS_VISIBLE | WS_CHILD, 220, 735, 100, 30, hwnd, (HMENU)ID_BTN_PREV_PG1, NULL, NULL);
        hBtnPage2 = CreateWindowW(L"BUTTON", L"Page 2", WS_VISIBLE | WS_CHILD, 330, 735, 100, 30, hwnd, (HMENU)ID_BTN_PREV_PG2, NULL, NULL);
        
        ApplyTheme(hwnd); UpdateUIFromSettings(); break;
    }
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 700; lpMMI->ptMinTrackSize.y = 860;
        break;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (hPreviewArea) {
            MoveWindow(hPreviewArea, 220, 10, w - 240, h - 100, TRUE);
            MoveWindow(hBtnPage1, 220, h - 70, 100, 30, TRUE);
            MoveWindow(hBtnPage2, 330, h - 70, 100, 30, TRUE);
            if (hGroupSettings) MoveWindow(hGroupSettings, 10, 5, 200, h - 20, TRUE);
            InvalidateRect(hPreviewArea, NULL, TRUE);
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG: {
        HDC hdc = (HDC)wParam;
        ThemeColors& t = g_themes[g_currentTheme];
        SetTextColor(hdc, t.text);
        SetBkColor(hdc, t.bg);
        return (LRESULT)t.hBrushBg;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        ThemeColors& t = g_themes[g_currentTheme];
        SetTextColor(hdc, t.text);
        SetBkColor(hdc, t.controlBg);
        return (LRESULT)t.hBrushControl;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        ThemeColors& t = g_themes[g_currentTheme];
        SetTextColor(hdc, t.text);
        SetBkColor(hdc, t.controlBg);
        return (LRESULT)t.hBrushControl;
    }
    case WM_DROPFILES: { HDROP hDrop = (HDROP)wParam; wchar_t file[MAX_PATH]; DragQueryFileW(hDrop, 0, file, MAX_PATH); HandleFileSelection(hwnd, 1, file); DragFinish(hDrop); break; }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_FRONT) HandleFileSelection(hwnd, 1, L"");
        if (LOWORD(wParam) == ID_BTN_BACK) HandleFileSelection(hwnd, 2, L"");
        if (LOWORD(wParam) == ID_MENU_IMPORT) ImportWorkspace(hwnd);
        if (LOWORD(wParam) == ID_MENU_EXPORT) ExportWorkspace(hwnd);
        if (LOWORD(wParam) == ID_MENU_THEME_LIGHT) { g_currentTheme = 0; SaveAppSettings(); ApplyTheme(hwnd); }
        if (LOWORD(wParam) == ID_MENU_THEME_DARK)  { g_currentTheme = 1; SaveAppSettings(); ApplyTheme(hwnd); }
        if (LOWORD(wParam) == ID_MENU_THEME_AZURE) { g_currentTheme = 2; SaveAppSettings(); ApplyTheme(hwnd); }
        if (LOWORD(wParam) == ID_MENU_ABOUT) MessageBoxW(hwnd, L"ID Card Print\nCreated by Maragung", L"About", MB_OK | MB_ICONINFORMATION);
        if (LOWORD(wParam) == ID_BTN_PRINT) {
            ReadSettingsFromUI(); if (g_imgFront == nullptr) { MessageBoxA(hwnd, "Please select the Front image.", "Error", MB_OK); break; }
            PRINTDLGW pd = {sizeof(pd), hwnd, NULL, NULL, NULL, PD_RETURNDC}; if (!PrintDlgW(&pd)) break;
            DOCINFOW di = { sizeof(DOCINFOW), L"ID Card Print", NULL, NULL, 0 };
            if (StartDocW(pd.hDC, &di) > 0) {
                int dpiX = GetDeviceCaps(pd.hDC, LOGPIXELSX), dpiY = GetDeviceCaps(pd.hDC, LOGPIXELSY), pOffX = GetDeviceCaps(pd.hDC, PHYSICALOFFSETX), pOffY = GetDeviceCaps(pd.hDC, PHYSICALOFFSETY);
                Graphics g(pd.hDC); g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                auto positions = CalculateLayout(g_settings); int currentPage = -1;
                for (const auto& pos : positions) {
                    if (pos.page != currentPage) { if (currentPage != -1) EndPage(pd.hDC); StartPage(pd.hDC); currentPage = pos.page; }
                    float xPx = (pos.x_mm / 25.4f) * dpiX - pOffX, yPx = (pos.y_mm / 25.4f) * dpiY - pOffY, wPx = (g_settings.cardWidthMm / 25.4f) * dpiX, hPx = (g_settings.cardHeightMm / 25.4f) * dpiY;
                    DrawIDCard(g, pos.isFront ? g_imgFront : g_imgBack, pos.isFront ? g_frontXform : g_frontXform, xPx, yPx, wPx, hPx, g_settings.cardWidthMm);
                }
                if (currentPage != -1) EndPage(pd.hDC); EndDoc(pd.hDC);
            }
            DeleteDC(pd.hDC); break;
        }
        if (LOWORD(wParam) == ID_BTN_PREV_PG1) { g_previewPage = 0; InvalidateRect(hPreviewArea, NULL, TRUE); }
        if (LOWORD(wParam) == ID_BTN_PREV_PG2) { g_previewPage = 1; InvalidateRect(hPreviewArea, NULL, TRUE); }
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == BN_CLICKED) { SaveAppSettings(); InvalidateRect(hPreviewArea, NULL, TRUE); }
        break;
    case WM_DESTROY: SaveAppSettings(); for(int i=0;i<3;++i) { if(g_themes[i].hBrushBg) DeleteObject(g_themes[i].hBrushBg); if(g_themes[i].hBrushControl) DeleteObject(g_themes[i].hBrushControl); } PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartupInput gsi; ULONG_PTR gst; GdiplusStartup(&gst, &gsi, NULL);
    WNDCLASSEXW wc = {0}; wc.cbSize = sizeof(WNDCLASSEXW); wc.lpfnWndProc = WndProc; wc.hInstance = hInstance; wc.lpszClassName = L"MainClass"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"MainClass", L"ID Card Print", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 720, 900, NULL, NULL, hInstance, NULL);
    HFONT hf = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Segoe UI");
    EnumChildWindows(hwnd, [](HWND c, LPARAM f){ SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hf);
    ShowWindow(hwnd, nCmdShow); MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GdiplusShutdown(gst); return 0;
}
