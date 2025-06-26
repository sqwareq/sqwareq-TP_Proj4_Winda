# Prosty Symulator Windy w C++ z użyciem WinAPI
# Julia Jachimowska 203722, Arkadiusz Róg 203856

## Opis projektu

Projekt jest graficzną symulacją działania windy pasażerskiej, zrealizowaną w języku C++ z wykorzystaniem natywnego API Windows (WinAPI) oraz biblioteki GDI+ do renderowania grafiki 2D. Aplikacja wizualizuje ruch windy w szybie, obsługuje kolejkę wezwań, pozwala na interakcję z użytkownikiem oraz implementuje logikę kontroli udźwigu i automatycznego powrotu na parter.

## Kluczowe Elementy Implementacji

Poniżej przedstawiono szczegółowe omówienie kluczowych aspektów programu wraz z fragmentami kodu źródłowego.

### Konfiguracja i stałe globalne

U podstaw symulacji leży zestaw stałych, które definiują parametry środowiska, takie jak liczba pięter, wymiary czy szybkość windy. Użycie stałych globalnych ułatwia konfigurację i ewentualne modyfikacje symulacji.

```cpp
const int LICZBA_PIETER = 5;
const int WYSOKOSC_PIETRA = 120;
const int MARGINES_GORA = 50;
const int POZIOM_PARTERU_Y = MARGINES_GORA + LICZBA_PIETER * WYSOKOSC_PIETRA;
const float PREDKOSC_WINDY = 2.5f;
const int MAKS_PASAZEROW = 8;
```

### Centralna struktura danych i stany windy

Cały bieżący stan windy przechowywany jest w pojedynczej strukturze `Winda`. Ułatwia to zarządzanie danymi i przekazywanie ich między funkcjami. Typ wyliczeniowy `StanWindy` definiuje maszynę stanów, która steruje logiką windy.

* `BEZCZYNNA` – winda stoi w miejscu i czeka na wezwania.
* `JEDZIE_GORA` / `JEDZIE_DOL` – winda jest w ruchu.
* `DRZWI_OTWARTE` – winda stoi na piętrze, umożliwiając wymianę pasażerów.

```cpp
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

Winda winda; // Globalna instancja windy
```

### Inicjalizacja okna i zasobów

Po utworzeniu głównego okna aplikacji (komunikat `WM_CREATE`), inicjalizowany jest początkowy stan windy. Winda ustawiana jest na parterze w stanie bezczynności. Tworzone są również przyciski do obsługi pasażerów oraz dwa kluczowe timery:

1.  `ID_TIMER_GLOWNY` – timer o wysokiej częstotliwości (16 ms, ~60 FPS) do obsługi animacji i głównej logiki.
2.  `ID_TIMER_BEZCZYNNSCI` – timer o niskiej częstotliwości (1000 ms) do sprawdzania warunku powrotu na parter.

```cpp
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
```

### Zadanie 3.1: Symulacja przewozu osób i kontrola udźwigu

Realizacja tego zadania opiera się na obsłudze przycisków `+` i `-` w ramach komunikatu `WM_COMMAND`.

Logika pozwala na dodawanie pasażerów tylko wtedy, gdy drzwi windy są otwarte (`winda.stan == DRZWI_OTWARTE`). Przed dodaniem nowej osoby sprawdzane jest, czy nie zostanie przekroczony limit `MAKS_PASAZEROW`. Jeśli winda jest pełna, użytkownik jest o tym informowany za pomocą okna dialogowego `MessageBox`. Każda zmiana liczby pasażerów resetuje licznik bezczynności.

```cpp
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
```

### Główna pętla logiki (Maszyna Stanów)

Obsługa `ID_TIMER_GLOWNY` zawiera kluczową logikę programu opartą na maszynie stanów.

#### Stan `BEZCZYNNA`: Wybór celu

Gdy winda jest bezczynna i istnieją oczekujące wezwania, uruchamiany jest algorytm wyboru celu. Przeszukuje on kolejkę wezwań (`std::set`), aby znaleźć piętro położone najbliżej aktualnej pozycji windy. Po znalezieniu celu jest on usuwany z kolejki, a stan windy zmieniany jest na ruch (`JEDZIE_GORA` lub `JEDZIE_DOL`).

```cpp
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

        if (najblizszyCel == winda.aktualnePietro) { // Jeśli wezwano nas na piętro, na którym stoimy
            winda.kolejkaWezwan.erase(najblizszyCel);
            winda.stan = DRZWI_OTWARTE;
            winda.licznikDrzwi = 180; // Ustawia czas otwarcia drzwi
        }
        else {
            winda.celPodrozy = najblizszyCel;
            winda.kolejkaWezwan.erase(najblizszyCel);
            winda.stan = (winda.celPodrozy > winda.aktualnePietro) ? JEDZIE_GORA : JEDZIE_DOL;
        }
    }
    break;
```

#### Stan `JEDZIE_GORA` / `JEDZIE_DOL`: Ruch windy

W stanach ruchu pozycja Y windy jest co klatkę aktualizowana o wartość `PREDKOSC_WINDY`. System stale sprawdza, czy winda dotarła do docelowej współrzędnej Y. Po dotarciu na miejsce pozycja jest korygowana do idealnej wartości, aktualizowane jest piętro, a stan zmieniany jest na `DRZWI_OTWARTE`.

```cpp
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
        winda.licznikDrzwi = 180; // Ustawia czas otwarcia drzwi (180 klatek * 16ms ~= 3s)
    }
    break;
}
```

### Zadanie 3.2: Mechanizm powrotu pustej windy na parter

Ten mechanizm jest realizowany przez drugi, wolniejszy timer `ID_TIMER_BEZCZYNNSCI`. Co sekundę sprawdza on, czy spełnione są łącznie trzy warunki: winda jest bezczynna, pusta i nie znajduje się na parterze. Jeśli tak, licznik `licznikBezczynnosci` jest inkrementowany. Po 5 sekundach (gdy licznik osiągnie 5) do kolejki wezwań dodawane jest piętro 0.

```cpp
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
```

### Zadanie 3.3: Prezentacja masy pasażerów na interfejsie

Wyświetlanie aktualnej liczby pasażerów oraz ich szacunkowej wagi odbywa się w funkcji `RysujScene`, która jest wywoływana przy każdym odświeżeniu ramki.

Waga jest obliczana jako iloczyn liczby pasażerów i średniej wagi (70 kg). Następnie tworzony jest sformatowany napis, który jest rysowany w lewym górnym rogu ekranu, w dedykowanym panelu informacyjnym.

```cpp
void RysujScene(HDC hdc) {
    // ... inicjalizacja i rysowanie tła, szybu, pięter ...

    // Rysowanie pasażerów w windzie
    for (int i = 0; i < winda.pasazerowie; ++i) {
        float osX = szybX + 20 + (i % 3) * 30;
        float osY = (winda.pozY + wysWindy) - 17 - ((i / 3) * 40);
        RysujLudka(graphics, penCzarny, osX, osY);
    }
    
    // Rysowanie panelu informacyjnego
    RectF panelRect(10, 10, 210, 80);
    graphics.DrawRectangle(&penCzarny, panelRect);
    graphics.DrawString(L"LICZBA OSÓB:", -1, &fontPanel, PointF(15, 18), &brushTekst);
    
    int waga = winda.pasazerowie * 70;
    std::wstring wagaStr = std::to_wstring(winda.pasazerowie) + L" (" + std::to_wstring(waga) + L"kg / " + std::to_wstring(MAKS_PASAZEROW * 70) + L"kg)";
    graphics.DrawString(wagaStr.c_str(), -1, &fontPanel, PointF(15, 50), &brushTekst);
}
```

### Renderowanie grafiki i podwójne buforowanie

Aby uniknąć migotania animacji, zastosowano technikę podwójnego buforowania. W obsłudze komunikatu `WM_PAINT` cała scena jest najpierw rysowana do niewidocznej bitmapy w pamięci (`hdcMem`), a dopiero gdy jest kompletna, jest kopiowana jednym ruchem na ekran (`BitBlt`).

```cpp
case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    // Utworzenie bufora w pamięci
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    SelectObject(hdcMem, hbmMem);

    // Rysowanie całej sceny do bufora
    RysujScene(hdcMem);

    // Skopiowanie gotowej klatki z bufora na ekran
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

    // Zwolnienie zasobów
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
    break;
}