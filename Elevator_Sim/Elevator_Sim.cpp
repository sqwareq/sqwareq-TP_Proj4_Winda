#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <set>
#include <cmath>

#pragma comment (lib, "gdiplus.lib")

using namespace Gdiplus;

const int LICZBA_PIETER = 5;
const int WYSOKOSC_PIETRA = 120;
const int MARGINES_GORA = 50;
const int POZIOM_PARTERU_Y = MARGINES_GORA + LICZBA_PIETER * WYSOKOSC_PIETRA;
const float PREDKOSC_WINDY = 2.5f;
const int MAKS_PASAZEROW = 8;

#define ID_TIMER_GLOWNY 1
#define ID_TIMER_BEZCZYNNSCI 2
#define ID_BTN_DODAJ 101
#define ID_BTN_USUN 102

enum StanWindy { BEZCZYNNA, JEDZIE_GORA, JEDZIE_DOL, DRZWI_OTWARTE };

struct Winda {
    float pozY;
    int aktualnePietro;
    StanWindy stan;
    int pasazerowie;
    int licznikDrzwi;
    int licznikBezczynnosci;
    std::set<int> kolejkaWezwan;
    int celPodrozy;
};

Winda winda;
HWND hBtnDodaj, hBtnUsun;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RysujScene(HDC hdc);
void RysujLudka(Graphics& g, Pen& pen, float x, float y);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"KlasaSymulatoraWindy";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Prosty Symulator Windy", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 800, 750, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        float wysWindy = WYSOKOSC_PIETRA - 5.0f;
        float startY = (float)(POZIOM_PARTERU_Y - 0 * WYSOKOSC_PIETRA) - wysWindy;
        winda = { startY, 0, BEZCZYNNA, 0, 0, 0, {}, -1 };

        hBtnDodaj = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE, 140, 45, 30, 30, hwnd, (HMENU)ID_BTN_DODAJ, NULL, NULL);
        hBtnUsun = CreateWindowW(L"BUTTON", L"-", WS_CHILD | WS_VISIBLE, 175, 45, 30, 30, hwnd, (HMENU)ID_BTN_USUN, NULL, NULL);

        SetTimer(hwnd, ID_TIMER_GLOWNY, 16, NULL);
        SetTimer(hwnd, ID_TIMER_BEZCZYNNSCI, 1000, NULL);
        break;
    }

    case WM_LBUTTONDOWN: {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        for (int i = 0; i < LICZBA_PIETER; ++i) {
            float promien = 20.0f;
            float srodekY = (float)(POZIOM_PARTERU_Y - i * WYSOKOSC_PIETRA) - 80.0f + promien;
            float srodekX;
            if (i % 2 == 0) srodekX = 270.0f + promien;
            else srodekX = 350.0f + 100.0f + 40.0f + promien;

            if (pow(mouseX - srodekX, 2) + pow(mouseY - srodekY, 2) < pow(promien, 2)) {
                winda.kolejkaWezwan.insert(i);
                winda.licznikBezczynnosci = 0;
                break;
            }
        }
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_BTN_DODAJ && winda.stan == DRZWI_OTWARTE) {
            if (winda.pasazerowie < MAKS_PASAZEROW) {
                winda.pasazerowie++;
                winda.licznikBezczynnosci = 0;
            }
            else {
                MessageBox(hwnd, L"Winda jest pełna!", L"Przeciążenie", MB_OK | MB_ICONWARNING);
            }
        }
        if (LOWORD(wParam) == ID_BTN_USUN && winda.stan == DRZWI_OTWARTE && winda.pasazerowie > 0) {
            winda.pasazerowie--;
            winda.licznikBezczynnosci = 0;
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == ID_TIMER_GLOWNY) {
            switch (winda.stan) {
            case BEZCZYNNA:
                if (!winda.kolejkaWezwan.empty()) {
                    int najblizszyCel = -1;
                    int minOdleglosc = 1000;

                    for (int pietro : winda.kolejkaWezwan) {
                        if (abs(pietro - winda.aktualnePietro) < minOdleglosc) {
                            minOdleglosc = abs(pietro - winda.aktualnePietro);
                            najblizszyCel = pietro;
                        }
                    }

                    if (najblizszyCel == winda.aktualnePietro) {
                        winda.kolejkaWezwan.erase(najblizszyCel);
                        winda.stan = DRZWI_OTWARTE;
                        winda.licznikDrzwi = 180;
                    }
                    else {
                        winda.celPodrozy = najblizszyCel;
                        winda.kolejkaWezwan.erase(najblizszyCel);
                        winda.stan = (winda.celPodrozy > winda.aktualnePietro) ? JEDZIE_GORA : JEDZIE_DOL;
                    }
                }
                break;

            case JEDZIE_GORA:
            case JEDZIE_DOL: {
                float wysWindy = WYSOKOSC_PIETRA - 5.0f;
                float doceloweY = (float)(POZIOM_PARTERU_Y - winda.celPodrozy * WYSOKOSC_PIETRA) - wysWindy;

                if (winda.stan == JEDZIE_GORA) winda.pozY -= PREDKOSC_WINDY;
                else winda.pozY += PREDKOSC_WINDY;

                if ((winda.stan == JEDZIE_GORA && winda.pozY <= doceloweY) ||
                    (winda.stan == JEDZIE_DOL && winda.pozY >= doceloweY)) {
                    winda.pozY = doceloweY;
                    winda.aktualnePietro = winda.celPodrozy;
                    winda.celPodrozy = -1;
                    winda.stan = DRZWI_OTWARTE;
                    winda.licznikDrzwi = 180;
                }
                break;
            }

            case DRZWI_OTWARTE:
                if (--winda.licznikDrzwi <= 0) {
                    winda.stan = BEZCZYNNA;
                }
                break;
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (wParam == ID_TIMER_BEZCZYNNSCI) {
            if (winda.stan == BEZCZYNNA && winda.pasazerowie == 0 && winda.aktualnePietro != 0) {
                if (++winda.licznikBezczynnosci >= 5) {
                    winda.kolejkaWezwan.insert(0);
                    winda.licznikBezczynnosci = 0;
                }
            }
            else {
                winda.licznikBezczynnosci = 0;
            }
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(hdcMem, hbmMem);

        RysujScene(hdcMem);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_GLOWNY);
        KillTimer(hwnd, ID_TIMER_BEZCZYNNSCI);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void RysujLudka(Graphics& g, Pen& pen, float x, float y) {
    g.DrawEllipse(&pen, x - 7, y - 25, 14.0f, 14.0f);
    g.DrawLine(&pen, x, y - 11, x, y + 5);
    g.DrawLine(&pen, x, y - 5, x - 10, y);
    g.DrawLine(&pen, x, y - 5, x + 10, y);
    g.DrawLine(&pen, x, y + 5, x - 8, y + 15);
    g.DrawLine(&pen, x, y + 5, x + 8, y + 15);
}

void RysujScene(HDC hdc) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush brushTlo(Color(255, 255, 255, 255));
    graphics.FillRectangle(&brushTlo, 0, 0, 800, 750);
    Pen penCzarny(Color(255, 0, 0, 0), 2);
    Pen penCzerwony(Color(255, 255, 0, 0), 3);
    Pen penSzary(Color(255, 128, 128, 128), 1);
    SolidBrush brushCzerwony(Color(255, 255, 0, 0));
    SolidBrush brushBialy(Color(255, 255, 255, 255));
    SolidBrush brushTekst(Color(255, 0, 0, 0));
    Font fontLiczby(L"Arial", 14, FontStyleBold);
    Font fontPanel(L"Calibri", 12, FontStyleBold);
    StringFormat formatSrodek;
    formatSrodek.SetAlignment(StringAlignmentCenter);
    formatSrodek.SetLineAlignment(StringAlignmentCenter);

    float szybX = 350.0f, szybSzer = 100.0f;

    graphics.DrawRectangle(&penCzarny, szybX, (float)MARGINES_GORA, szybSzer, (float)(LICZBA_PIETER * WYSOKOSC_PIETRA));

    for (int i = 0; i < LICZBA_PIETER; ++i) {
        float y = (float)(POZIOM_PARTERU_Y - i * WYSOKOSC_PIETRA);
        graphics.DrawLine(&penSzary, szybX - 120, y, szybX + szybSzer + 120, y);

        float koloX;
        if (i % 2 == 0) koloX = 270.0f;
        else koloX = szybX + szybSzer + 40.0f;

        RectF koloRect(koloX, y - 80, 40, 40);
        graphics.FillEllipse(&brushCzerwony, koloRect);

        if (winda.kolejkaWezwan.count(i)) {
            Pen penZaznaczenie(Color(180, 255, 255, 0), 4);
            graphics.DrawEllipse(&penZaznaczenie, koloRect);
        }

        std::wstring strPietro = (i == 0) ? L"P" : std::to_wstring(i);
        graphics.DrawString(strPietro.c_str(), -1, &fontLiczby, koloRect, &formatSrodek, &brushBialy);

        float ludekY = y - 15;
        if (i == 0) RysujLudka(graphics, penCzarny, szybX + szybSzer + 50, ludekY);
        if (i == 1) RysujLudka(graphics, penCzarny, szybX - 50, ludekY);
        if (i == 2) RysujLudka(graphics, penCzarny, szybX + szybSzer + 50, ludekY);
        if (i == 3) RysujLudka(graphics, penCzarny, szybX - 50, ludekY);
        if (i == 4) RysujLudka(graphics, penCzarny, szybX + szybSzer + 50, ludekY);
    }

    float wysWindy = WYSOKOSC_PIETRA - 5.0f;
    RectF windaRect(szybX, winda.pozY, szybSzer, wysWindy);
    graphics.DrawRectangle(&penCzerwony, windaRect);

    for (int i = 0; i < winda.pasazerowie; ++i) {
        float osX = szybX + 20 + (i % 3) * 30;
        float osY = (winda.pozY + wysWindy) - 17 - ((i / 3) * 40);
        RysujLudka(graphics, penCzarny, osX, osY);
    }

    RectF panelRect(10, 10, 210, 80);
    graphics.DrawRectangle(&penCzarny, panelRect);
    graphics.DrawString(L"LICZBA OSÓB:", -1, &fontPanel, PointF(15, 18), &brushTekst);
    int waga = winda.pasazerowie * 70;
    std::wstring wagaStr = std::to_wstring(winda.pasazerowie) + L" (" + std::to_wstring(waga) + L"kg / " + std::to_wstring(MAKS_PASAZEROW * 70) + L"kg)";
    graphics.DrawString(wagaStr.c_str(), -1, &fontPanel, PointF(15, 50), &brushTekst);
}