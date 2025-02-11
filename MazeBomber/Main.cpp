#include "icb_gui.h"
#include <cstdlib>
#include <vector>
#include <queue>
#include <unordered_map>
#include <chrono>
#include <windows.h>
/*********************************************VARIABLES******************************************************************/
int FRM1;
int winnerIndex = -1;
ICBYTES panel;
ICBYTES  logo;
bool isMenu = true;
bool fromControls = false;
bool thread_continue = false;
bool isPaused = false; // Oyunun duraklat�l�p duraklat�lmad���n� takip eden de�i�ken
bool single = false;
bool isInGameMenu = false;
bool isWinScreen = false;
bool keyStatesPlayer1[256] = { false }; // Mavi oyuncu tu� durumlar�
bool keyStatesPlayer2[256] = { false }; // K�rm�z� oyuncu tu� durumlar�
bool keyStatesPlayerOnline[256] = { false }; // Online i�in tu�lar

const int WIDTH = 15;
const int HEIGHT = 15;
const int cellSize = 50;
float minRatio = 0.55, maxRatio = 0.85;

bool inMainMenu = false;
bool isCalledFromMain = false;
bool isInOnlineMenu = false;
bool isInGameMode = false;
bool isInControls = false;
bool showLogoActive = true; // Logonun aktif olup olmad���n� kontrol et
std::chrono::time_point<std::chrono::steady_clock> logoStartTime;


std::chrono::time_point<std::chrono::steady_clock> shieldTimers[4]; // Kalkan zamanlar�n� tut
bool isShieldActive[4] = { false, false, false, false }; // Kalkan�n aktif olup olmad���n� takip et

std::chrono::time_point<std::chrono::steady_clock> trapActivationTimer[HEIGHT][WIDTH]; // Tuzak kurulunca aktif olma s�resi 3sn
std::chrono::time_point<std::chrono::steady_clock> curseTimer[4]; // Tuza�a d���nce yava�lama 7sn
bool isTrapActive = false; // tuza��n aktifle�mesini denetler
bool isTrapped[4] = { false, false, false, false }; // Yava�lamay� denetler

HANDLE Threads[4] = { NULL, NULL, NULL, NULL }; // Threadlerin y�netilmesi i�in
HANDLE menuThread[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
// ana men�     play    single      online      se�enek     oyun i�i    win screen

#define MAIN_MENU_INDEX 0
#define GAME_MODE_INDEX 1
#define SINGLE_MENU_INDEX 2
#define ONLINE_MENU_INDEX 3
#define SETTINGS_MENU_INDEX 4
#define IN_GAME_MENU_INDEX 5
#define WIN_SCREEN_INDEX 6


int multiplier[4][6];
int speed[4];
bool isTrapHidden[WIDTH][HEIGHT]; // T�m tuzaklar ba�lang��ta g�r�n�r
bool isTrapActiveGrid[WIDTH][HEIGHT] = { false }; // Her h�crede tuzak aktif mi?


char powerUpMap[WIDTH * HEIGHT];
char map[WIDTH * HEIGHT];
int playersPos[4][2];

ICBYTES speedIco, shieldPow, shieldIn, range, numOfBomb, debuff, trapIco, tugla, tuzak, tuzakIco, tuzak_kuruldu, bomba, mezar;


struct Bomb {
    int x, y;
    int owner; // Bombay� yerle�tiren oyuncu
    int timer; // Patlama zamanlay�c�s�
    int range; // Patlama alan�
};
std::vector<Bomb> bombs;

// A* Node
struct Node {
    int x, y;
    int cost, priority;
    bool operator<(const Node& other) const { return priority > other.priority; }
};


struct ExplosionEffect {
    int x, y;
    int direction; // 0:up, 1:down, 2:right, 3:left
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};

std::vector<ExplosionEffect> explosions; // T�m patlama efektleri
ICBYTES patlamaMerkez, patlamaOrtaDikey, patlamaOrtaYatay,patlamaUcYukari, patlamaUcAsagi, patlamaUcSol, patlamaUcSag;


/***********************************************FUNCTIONS******************************************************************/
void mainMenu();
void gameMode();
void inGameMenu();
void controlsMenu();
void drawExplosions();
void stopMenu(int currentIndex);
void RestartGame();
void StopThreads(int n);
/***********************************************MAP******************************************************************/
void drawShieldIndicator(int playerIndex) {
    if (multiplier[playerIndex][3] == 2) { // E�er oyuncunun kalkan� varsa
        int x = playersPos[playerIndex][0];
        int y = playersPos[playerIndex][1];
        // FillCircle(panel, x, y, cellSize / 8, 0xFFFFFF); //Kalkan�n var oldu�unuu g�stermek i�in yuvarlak
        PasteNon0(shieldIn, x, y - 10, panel);
    }
}

void drawTrapIndicator(int playerIndex) {
    if (multiplier[playerIndex][0] < 0) { // E�er oyuncu tuza�a d��t�yse
        int x = playersPos[playerIndex][0];
        int y = playersPos[playerIndex][1];
        PasteNon0(tuzakIco, x - 20, y - 10, panel);

    }
}
bool drawnWalls[WIDTH * HEIGHT] = { false }; // Hangi h�crelere tu�la �izildi�ini takip eder

void drawMap() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int xPos = x * cellSize;
            int yPos = y * cellSize;

            if (map[y * WIDTH + x] == '#') {
                FillRect(panel, xPos, yPos, cellSize, cellSize, 0x808080); // K�r�lmaz duvar
            }
            else if (map[y * WIDTH + x] == '+') {
                if (!drawnWalls[y * WIDTH + x]) { // E�er daha �nce �izilmediyse
                    PasteNon0(tugla, xPos, yPos, panel);
                    drawnWalls[y * WIDTH + x] = true; // Bir kez �izildi�ini i�aretle
                }
            }
            else if (map[y * WIDTH + x] == ' ') {
                FillRect(panel, xPos, yPos, cellSize, cellSize, 0x008001); // YOL
            }
            else if (map[y * WIDTH + x] == 'M') {
                FillRect(panel, xPos, yPos, cellSize - 1, cellSize - 1, 0x008001); // YOL
                PasteNon0(mezar, xPos, yPos, panel);
            }
            else if (map[y * WIDTH + x] == 'H') {
                if (isTrapActiveGrid[y][x]) {
                    FillRect(panel, xPos, yPos, cellSize, cellSize, 0x008000); // Tuzak aktif, g�r�nmez
                }
                else {
                    FillRect(panel, xPos, yPos, cellSize, cellSize, 0x008000); // Arkaplan
                    PasteNon0(tuzak_kuruldu, xPos, yPos, panel);// Tuzak yeni koyulmu�, g�r�n�r
                }
            }


            // G��lendirici kontrol�
            if (strchr("SRNKDT", map[y * WIDTH + x])) {
                int powerUpX = xPos + cellSize / 2;
                int powerUpY = yPos + cellSize / 2;

                switch (map[y * WIDTH + x]) {
                case 'S': PasteNon0(speedIco, xPos, yPos, panel); break;
                case 'R': PasteNon0(range, xPos, yPos, panel); break;
                case 'N': PasteNon0(numOfBomb, xPos, yPos, panel); break;
                case 'K': PasteNon0(shieldPow, xPos, yPos, panel); break;
                case 'D': PasteNon0(debuff, xPos, yPos, panel); break;
                case 'T': PasteNon0(tuzak, xPos, yPos, panel); break;
                }
            }
        }

    }

    // Bombalar� �iz
    for (const auto& bomb : bombs) {
        int xPos = bomb.x * cellSize;
        int yPos = bomb.y * cellSize;
        FillRect(panel, xPos, yPos, cellSize - 1, cellSize - 1, 0x008001); // YOL
        PasteNon0(bomba, xPos, yPos, panel);// bomba
    }

    // Oyuncular� �iz
    for (int i = 0; i < 4; i++) {
        if (playersPos[i][0] >= 0 && playersPos[i][1] >= 0) {
            int color = (i == 0) ? 0x0000FF : (i == 1) ? 0xFF0000 : (i == 2) ? 0x00FF00 : 0xFFFF00;
            FillCircle(panel, playersPos[i][0], playersPos[i][1], cellSize / 3, color);
            drawShieldIndicator(i);
            drawTrapIndicator(i);
        }
    }
    drawExplosions();
    DisplayImage(FRM1, panel);
}

void fixBrickOverlap(int px, int py) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int x = px + dx;
            int y = py + dy;

            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                int xPos = x * cellSize;
                int yPos = y * cellSize;

                if (map[y * WIDTH + x] == '+') {
                    PasteNon0(tugla, xPos, yPos, panel); // Tu�lay� tekrar �iz
                }
            }
        }
    }
}


void initializeMap(float minRatio, float maxRatio) {

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 6; j++) {
            multiplier[i][j] = 1;
        }
    }

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            isTrapHidden[i][j] = false;
            isTrapActiveGrid[i][j] = false;
        }

    }

    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        powerUpMap[i] = 'X';
    }

    for (int i = 0; i < 4; ++i) {
        isTrapped[i] = false;
        isShieldActive[i] = false;
    }
    bombs.clear();

    // Haritay� ba�lang�� de�erleriyle doldur
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                map[y * WIDTH + x] = '#'; // K�r�lamaz bloklar
            }
            else if (y % 2 == 0 && x % 2 == 0) {
                map[y * WIDTH + x] = '#'; // K�r�lamaz bloklar
            }
            else {
                map[y * WIDTH + x] = ' '; // Yol
            }
        }
    }


    // K��elerdeki 3x3 b�lgelerde k�r�lmaz bloklar korunur ve k�r�labilir blok eklenmez
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (map[y * WIDTH + x] != '#') map[y * WIDTH + x] = ' ';
            if (map[y * WIDTH + (WIDTH - 1 - x)] != '#') map[y * WIDTH + (WIDTH - 1 - x)] = ' ';
            if (map[(HEIGHT - 1 - y) * WIDTH + x] != '#') map[(HEIGHT - 1 - y) * WIDTH + x] = ' ';
            if (map[(HEIGHT - 1 - y) * WIDTH + (WIDTH - 1 - x)] != '#') map[(HEIGHT - 1 - y) * WIDTH + (WIDTH - 1 - x)] = ' ';
        }
    }


    // Haritadaki toplam ' ' (yol) say�s�n� hesapla
    int totalPaths = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (map[i] == ' ') {
            totalPaths++;
        }
    }

    // Rastgele oran belirle ve doldurulacak blok say�s�n� hesapla
    double randomRatio = minRatio + (static_cast<double>(rand()) / RAND_MAX) * (maxRatio - minRatio);
    int numBlocks = static_cast<int>(totalPaths * randomRatio);

    int replaced = 0;
    while (replaced < numBlocks) {
        int x = rand() % WIDTH;
        int y = rand() % HEIGHT;
        int index = y * WIDTH + x;

        // Sadece ' ' olan h�creleri '+' ile de�i�tir, k��e b�lgelerine ve k�r�lamaz bloklara dokunma
        if (map[index] == ' ' && !((x < 3 && y < 3) || (x >= WIDTH - 3 && y < 3) || (x < 3 && y >= HEIGHT - 3) || (x >= WIDTH - 3 && y >= HEIGHT - 3))) {
            map[index] = '+';
            replaced++;
        }
    }

    replaced = 0;
    while (replaced < numBlocks * 0.2 ) { //POWER UP BEL�RLEME
        int x = rand() % WIDTH;
        int y = rand() % HEIGHT;
        int index = y * WIDTH + x;

        if (map[index] == '+' && powerUpMap[index] == 'X') {
            //{ 'S', 'R', 'N', 'K', 'D' } Speed, Bomb Range, NumOf Bomb, Kalkan, Debuff

            int p = rand() % 6;

            switch (p)
            {
            case 0: //Speed 
                powerUpMap[index] = 'S';
                break;

            case 1: //Range of bomb
                powerUpMap[index] = 'R';
                break;

            case 2: // Number Of Bomb
                powerUpMap[index] = 'N';
                break;

            case 3: // Shield
                powerUpMap[index] = 'K';
                break;

            case 4: //Debuff
                powerUpMap[index] = 'D';
                break;

            case 5: //Trap
                powerUpMap[index] = 'T';
                break;
            }

            ++replaced;
        }

    }

}

void winScreen() {
    const char* colors[4] = { "blue.bmp", "red.bmp", "green.bmp", "yellow.bmp" };
    stopMenu(WIN_SCREEN_INDEX);
    isWinScreen = true;
    StopThreads(single ? 4 : 2);
    ReadImage(colors[winnerIndex], logo);
    DisplayImage(FRM1, logo);
    Sleep(500);
}

/********************************************COLLISION************************************************************/
bool canMoveTo(int x, int y, int playerIndex) {
    int cellX = x / cellSize; // Pozisyonu h�cre sistemine �evir
    int cellY = y / cellSize;
    int currentX = playersPos[playerIndex][0] / cellSize; // Oyuncunun �u anki h�cresi (X)
    int currentY = playersPos[playerIndex][1] / cellSize; // Oyuncunun �u anki h�cresi (Y)

    // Harita s�n�rlar� i�inde mi?
    if (cellX < 0 || cellX >= WIDTH || cellY < 0 || cellY >= HEIGHT) {
        return false;
    }

    // Oyuncunun �u an bulundu�u h�crede bomba var m�?
    bool isOnBomb = (map[currentY * WIDTH + currentX] == 'B' && map[cellY * WIDTH + cellX] != '+' && map[cellY * WIDTH + cellX] != '#');

    // E�er oyuncu bomban�n �zerindeyse, hareket etmesine izin ver
    if (isOnBomb) {
        return true;
    }

    // E�er oyuncu bomban�n �zerinde de�ilse ve gitmeye �al��t��� h�crede bomba varsa, ge�i�i engelle
    if (map[cellY * WIDTH + cellX] == 'B') {
        return false;
    }

    // Haritan�n bu h�cresinde ge�ilebilir bir alan var m�?
    return map[cellY * WIDTH + cellX] != '+' && map[cellY * WIDTH + cellX] != '#';
}
/********************************************BOMB***************************************************************/
void placeBomb(int player) {
    const int DEFAULT_BOMB_TIMER = 100;
    int x = playersPos[player][0] / cellSize;
    int y = playersPos[player][1] / cellSize;

    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

    char cell = map[y * WIDTH + x];
    if (cell == '#' || cell == '+') return;

    // Oyuncunun ka� bomba koydu�unu say
    int bombCount = 0;
    for (const auto& bomb : bombs) {
        if (bomb.owner == player) {
            bombCount++;
        }
    }

    if (bombCount >= multiplier[player][2]) return;

    // Ayn� h�crede zaten bir bomba var m� kontrol et
    bool bombExists = false;
    for (const auto& bomb : bombs) {
        if (bomb.x == x && bomb.y == y) {
            bombExists = true;
            break;
        }
    }

    if (!bombExists) {
        bombs.push_back({ x, y, player, DEFAULT_BOMB_TIMER, multiplier[player][1] });
        map[y * WIDTH + x] = 'B';
    }
}
void placeStrategicBomb(int botIndex) {
    int botX = playersPos[botIndex][0] / cellSize;
    int botY = playersPos[botIndex][1] / cellSize;

    placeBomb(botIndex); // Bomba koy
    Sleep(200); // Patlama s�resini bekle
}


void handleExplosionOnPlayers(int nx, int ny) {
    int alivePlayers = 0;

    for (int i = 0; i < 4; i++) {
        if ((playersPos[i][0] / cellSize == nx && playersPos[i][1] / cellSize == ny) && multiplier[i][3] == 1) {
            playersPos[i][0] = -1; // Oyuncunun X pozisyonunu ge�ersiz yap
            playersPos[i][1] = -1; // Oyuncunun Y pozisyonunu ge�ersiz yap
            map[ny * WIDTH + nx] = 'M';
        }

        // Hayatta olan oyuncular� say ve kazanan� belirle
        if (playersPos[i][0] >= 0 && playersPos[i][1] >= 0) {
            alivePlayers++;
            winnerIndex = i;
        }
    }

    // E�er sadece bir oyuncu kald�ysa oyunu bitir
    if (alivePlayers == 1) {
        menuThread[WIN_SCREEN_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)winScreen, NULL, 0, NULL);
    }
}

void drawExplosions() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = explosions.begin(); it != explosions.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->startTime).count();
        if (elapsed > 500) { // 0.5 saniye g�ster
            it = explosions.erase(it); // Efekti sil
        }
        else {
            // Efekt tipine g�re �izim yap
            int x = it->x * cellSize;
            int y = it->y * cellSize;

            switch (it->direction) {
            case -1: // Merkez
                PasteNon0(patlamaMerkez, x, y, panel);
                break;
            case -2: // Orta dikey
                PasteNon0(patlamaOrtaDikey, x, y, panel);
                break;
            case -3: // Orta yatay
                PasteNon0(patlamaOrtaYatay, x, y, panel);
                break;
            case 0: // Yukar�
                PasteNon0(patlamaUcYukari, x, y, panel);
                break;
            case 1: // Sa�
                PasteNon0(patlamaUcSag, x, y, panel);
                break;
            case 2: // A�a��
                PasteNon0(patlamaUcAsagi, x, y, panel);
                break;
            case 3: // Sol
                PasteNon0(patlamaUcSol, x, y, panel);
                break;
            }
            ++it;
        }
    }
}
void explodeBomb(Bomb& bomb) {
    const int dx[] = { 0, 0, 1, 0, -1 }; // merkez, yukar�, sa�a, a�a��, sola
    const int dy[] = { 0, -1, 0, 1, 0 };

    // Bomban�n oldu�u h�creyi temizle
    map[bomb.y * WIDTH + bomb.x] = ' '; // Bomba patlad���nda haritadan kald�r

    // Merkez patlama efekti ekle (sadece bir kez)
    explosions.push_back({ bomb.x, bomb.y, -1, std::chrono::steady_clock::now() });

    for (int d = 1; d < 5; d++) { // d = 1'den ba�la (merkez hari�)
        bool blocked = false; // Patlaman�n bu y�nde devam edip etmedi�ini kontrol et
        for (int step = 1; step <= bomb.range && !blocked; step++) { // Bomban�n menzili kadar ilerle
            int nx = bomb.x + dx[d] * step; // Yeni X pozisyonu
            int ny = bomb.y + dy[d] * step; // Yeni Y pozisyonu

            // Harita s�n�rlar�n� kontrol et
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
                blocked = true; // Harita d���na ��kt�ysa patlama durdurulur
                break;
            }

            char& cell = map[ny * WIDTH + nx]; // Haritan�n ilgili h�cresine referans

            if (cell == '#') {
                blocked = true; // K�r�lmaz blok, patlamay� durdurur
                continue; // K�r�lmaz bloklara efekt koyma
            }
            else if (cell == '+') { // K�r�labilir blok
                if (powerUpMap[ny * WIDTH + nx] != 'X') {
                    cell = powerUpMap[ny * WIDTH + nx]; // G��lendirici varsa onu yerle�tir
                }
                else {
                    cell = ' '; // Blok k�r�l�r
                }
                blocked = true; // K�r�labilir blok patlamay� durdurur
            }
            else if (cell == 'S' || cell == 'R' || cell == 'N' || cell == 'K' || cell == 'D' || cell == 'T') {
                cell = ' '; // G�� par�as�n� kald�r
                powerUpMap[ny * WIDTH + nx] = 'X';
                blocked = true; // G�� par�as� �zerinde dur
            }

            // Patlama efekti ekle
            if (step == bomb.range) {
                // Patlaman�n sonunda u� efektler
                explosions.push_back({ nx, ny, d - 1, std::chrono::steady_clock::now() }); // d - 1: y�n� ayarla
            }
            else {
                // Orta k�s�mlar i�in yatay/dikey efektler
                if (d == 1 || d == 3) { // Yukar� veya a�a��
                    explosions.push_back({ nx, ny, -2, std::chrono::steady_clock::now() }); // Dikey orta
                }
                else if (d == 2 || d == 4) { // Sa� veya sol
                    explosions.push_back({ nx, ny, -3, std::chrono::steady_clock::now() }); // Yatay orta
                }
            }

            // Oyuncular�n patlama etkisini kontrol et
            handleExplosionOnPlayers(nx, ny);
        }
    }
}
std::vector<std::pair<int, int>> getBombEffectArea(const Bomb& bomb) {
    std::vector<std::pair<int, int>> affectedCells;
    const int dx[] = { 0, 0, 1, 0, -1 }; // Sa�, Yukar�, A�a��, Sol
    const int dy[] = { 0, -1, 0, 1, 0 };

    for (int d = 0; d < 4; d++) { // 4 y�n
        for (int step = 1; step <= bomb.range; step++) {
            int nx = bomb.x + dx[d] * step;
            int ny = bomb.y + dy[d] * step;

            // Harita s�n�rlar�n� kontrol et
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
                break;
            }

            char cell = map[ny * WIDTH + nx];
            if (cell == '#') {
                break; // K�r�lmaz blok varsa patlama durur
            }

            affectedCells.push_back({ nx, ny }); // Etkilenen h�creyi ekle
        }
    }

    // Merkez h�creyi de ekle
    affectedCells.push_back({ bomb.x, bomb.y });

    return affectedCells;
}

bool isSafe(int x, int y) {
    for (const auto& bomb : bombs) {
        auto affectedCells = getBombEffectArea(bomb);

        for (const auto& cell : affectedCells) {
            if (cell.first == x && cell.second == y) {
                return false; // E�er h�cre bomba etkisi alt�ndaysa g�vensiz
            }
        }
    }
    return true; // G�venli h�cre
}


/************************************** POWER UP *************************************************************/
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < 4; i++) {
        if (isShieldActive[i]) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - shieldTimers[i]).count();
            if (elapsed >= 10) { // E�er 10 saniye ge�tiyse kalkan� kald�r
                multiplier[i][3] = 1;
                isShieldActive[i] = false;
            }
        }
    }
}

void CALLBACK TimerTrap(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    auto now = std::chrono::steady_clock::now();

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (map[y * WIDTH + x] == 'H' && !isTrapActiveGrid[y][x]) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - trapActivationTimer[y][x]).count();
                if (elapsed >= 2) {
                    isTrapActiveGrid[y][x] = true; // Sadece bu h�credeki tuza�� aktif et
                }
            }
        }
    }
}


void CALLBACK TimerActivateTrap(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < 4; i++) {
        if (isTrapped[i]) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - curseTimer[i]).count();
            if (elapsed > 2 && elapsed < 7) {
                multiplier[i][0] = -2;
            }
            else if (elapsed >= 7) {
                // Oyuncunun h�z�n� geri ver ve tuzak etkisini kald�r
                multiplier[i][0] = speed[i];
                isTrapped[i] = false;
            }
        }
    }
}



void setupTimer(HWND hwnd) {
    SetTimer(hwnd, 1, 1000, TimerProc); // Her 1 saniyede bir `TimerProc` �al��acak
    SetTimer(hwnd, 1, 1000, TimerTrap); // Her 1 saniyede bir `TimerProc` �al��acak
    SetTimer(hwnd, 1, 1000, TimerActivateTrap); // Her 1 saniyede bir `TimerProc` �al��acak

}

void stepIntoTheTrap(int playerIndex, int index) {
    int x = index % WIDTH;
    int y = index / WIDTH;

    // E�er oyuncu zaten tuza�a d��m��se, �nce yava�lamay� kald�r
    if (isTrapped[playerIndex]) {
        multiplier[playerIndex][0] = speed[playerIndex]; // Eski h�z�n� geri ver
        isTrapped[playerIndex] = false; // Yava�lama etkisini kald�r
    }

    // Yeni tuzak etkisini ba�lat
    speed[playerIndex] = multiplier[playerIndex][0]; // Mevcut h�z�n� kaydet
    multiplier[playerIndex][0] = -3; // Yava�latma etkisi uygula
    curseTimer[playerIndex] = std::chrono::steady_clock::now(); // Yeni s�reyi ba�lat
    isTrapped[playerIndex] = true; // Tuzakta oldu�unu i�aretle
    map[index] = ' '; // Tuzak art�k aktif de�il, haritadan kald�r
}


void placeTrap(int player) {
    int x = playersPos[player][0] / cellSize;
    int y = playersPos[player][1] / cellSize;

    if (map[y * WIDTH + x] == ' ') {
        map[y * WIDTH + x] = 'H';
        isTrapActiveGrid[y][x] = false; // Tuzak ilk ba�ta g�r�n�r
        trapActivationTimer[y][x] = std::chrono::steady_clock::now(); // Her tuzak i�in ba��ms�z s�re ba�lat
    }
    multiplier[player][5] = 1;
}




void collectPowerUp(int index, int playerIndex) {

    // Power-up'� uygula
    switch (powerUpMap[index]) {
    case 'S': // Speed power-up
        if (!isTrapped[playerIndex]) {
            multiplier[playerIndex][0] += 0.5;
            if (multiplier[playerIndex][0] > 4) {
                multiplier[playerIndex][0] = 4; // Maksimum h�z 4x
            }
        }
        break;

    case 'R': // Bomb range power-up
        multiplier[playerIndex][1]++;
        if (multiplier[playerIndex][1] > 3) {
            multiplier[playerIndex][1] = 3; // Maksimum menzil 3
        }
        break;

    case 'N': // Number of bombs power-up
        multiplier[playerIndex][2]++;
        if (multiplier[playerIndex][2] > 4) {
            multiplier[playerIndex][2] = 4; // Maksimum bomba 4
        }
        break;

    case 'K': // Shield (Kalkan)
        multiplier[playerIndex][3] = 2; // Kalkan� aktif hale getir
        shieldTimers[playerIndex] = std::chrono::steady_clock::now(); // S�reyi s�f�rla
        isShieldActive[playerIndex] = true;
        break;

    case 'T': //Trap
        if (multiplier[playerIndex][5] == 1) { // E�er oyuncunun zaten kalkan� yoksa
            multiplier[playerIndex][5] = 2;
        }
        break;

    case 'D': // Debuff
        int i = rand() % 4; // Rastgele bir g��lendirmeyi d���r
        if (multiplier[playerIndex][i] > 1) {
            multiplier[playerIndex][i]--; // G��lendirmeyi azalt
        }
        else if (multiplier[playerIndex][i] <= 1 && i != 3) {
            multiplier[playerIndex][i] = 1; // Minimum de�eri koru
        }
        if (i == 3 && multiplier[playerIndex][3] == 2) { // E�er debuff kalkan� etkiliyorsa
            multiplier[playerIndex][3] = 1; // Kalkan� kapat
            isShieldActive[playerIndex] = false;
        }
        break;
    }

    powerUpMap[index] = 'X';
    map[index] = ' ';
}


/***************************************PLAYERS******************************************************************/
void movePlayer1() {
    while (thread_continue) {
        int x = playersPos[0][0];
        int y = playersPos[0][1];
        int cellX = x / cellSize;
        int cellY = y / cellSize;

        int replacement = 3 + multiplier[0][0];

        if (keyStatesPlayer1['W'] && canMoveTo(x, y - replacement, 0)) { // Yukar� hareket
            playersPos[0][1] -= replacement;
        }
        if (keyStatesPlayer1['A'] && canMoveTo(x - replacement, y, 0)) { // Sola hareket
            playersPos[0][0] -= replacement;
        }
        if (keyStatesPlayer1['S'] && canMoveTo(x, y + replacement, 0)) { // A�a�� hareket
            playersPos[0][1] += replacement;
        }
        if (keyStatesPlayer1['D'] && canMoveTo(x + replacement, y, 0)) { // Sa�a hareket
            playersPos[0][0] += replacement;
        }

        if (keyStatesPlayer1['Q'] && multiplier[0][5] == 2) { // Tuzak Kur
            placeTrap(0);
        }


        // G�ncel pozisyonu al
        x = playersPos[0][0];
        y = playersPos[0][1];

        if (powerUpMap[cellY * WIDTH + cellX] != 'X') {
            collectPowerUp((cellY * WIDTH + cellX), 0);
        }

        if (map[cellY * WIDTH + cellX] == 'H' && isTrapActiveGrid[cellY][cellX]) {
            stepIntoTheTrap(0, cellY * WIDTH + cellX);
        }

        for (auto& bomb : bombs) {
            if (--bomb.timer == 0) {
                PlaySound("bomb.wav", NULL, SND_ASYNC);
                explodeBomb(bomb);
                bomb = bombs.back();
                bombs.pop_back();
            }
        }
        fixBrickOverlap(cellX, cellY);
        drawMap();
        Sleep(30); // Ak�c� hareket i�in k�sa bir gecikme
    }
}


void movePlayer2() {
    while (thread_continue) {
        int x = playersPos[1][0];
        int y = playersPos[1][1];
        int cellX = x / cellSize;
        int cellY = y / cellSize;

        int replacement = 3 + multiplier[1][0];

        if (keyStatesPlayer2[VK_UP] && canMoveTo(x, y - replacement, 1)) { // Yukar� hareket
            playersPos[1][1] -= replacement;
        }
        if (keyStatesPlayer2[VK_LEFT] && canMoveTo(x - replacement, y, 1)) { // Sola hareket
            playersPos[1][0] -= replacement;
        }
        if (keyStatesPlayer2[VK_DOWN] && canMoveTo(x, y + replacement, 1)) { // A�a�� hareket
            playersPos[1][1] += replacement;
        }
        if (keyStatesPlayer2[VK_RIGHT] && canMoveTo(x + replacement, y, 1)) { // Sa�a hareket
            playersPos[1][0] += replacement;
        }

        if (keyStatesPlayer2[VK_NUMPAD3] && multiplier[1][5] == 2) { // Tuzak Kur
            placeTrap(1);
        }

        // G�ncel pozisyonu al
        x = playersPos[1][0];
        y = playersPos[1][1];

        if (powerUpMap[cellY * WIDTH + cellX] != 'X') {
            collectPowerUp((cellY * WIDTH + cellX), 1);

        }

        if (map[cellY * WIDTH + cellX] == 'H' && isTrapActiveGrid[cellY][cellX]) {
            stepIntoTheTrap(1, cellY * WIDTH + cellX);
        }

        fixBrickOverlap(cellX, cellY);
        Sleep(30); // Ak�c� hareket i�in k�sa bir gecikme
    }
}

/**********************************************BOTS**************************************************************/

std::vector<std::pair<int, int>> aStarPathfinding(int startX, int startY, int goalX, int goalY) {
    std::priority_queue<Node> frontier;
    frontier.push({ startX, startY, 0, 0 });

    std::unordered_map<int, int> cameFrom; // Parent d���m bilgisi
    std::unordered_map<int, int> costSoFar; // Maliyeti
    cameFrom[startY * WIDTH + startX] = -1;
    costSoFar[startY * WIDTH + startX] = 0;

    const int dx[] = { 0, 1, 0, -1 };
    const int dy[] = { 1, 0, -1, 0 };

    while (!frontier.empty()) {
        Node current = frontier.top();
        frontier.pop();

        // Hedefe ula��ld� m�?
        if (current.x == goalX && current.y == goalY) {
            std::vector<std::pair<int, int>> path;
            int index = goalY * WIDTH + goalX;
            while (cameFrom[index] != -1) {
                path.push_back({ index % WIDTH, index / WIDTH });
                index = cameFrom[index];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // Kom�u h�creleri kontrol et
        for (int i = 0; i < 4; i++) {
            int nextX = current.x + dx[i];
            int nextY = current.y + dy[i];

            if (nextX < 0 || nextX >= WIDTH || nextY < 0 || nextY >= HEIGHT) continue;
           // if (map[nextY * WIDTH + nextX] == '#' || map[nextY * WIDTH + nextX] == '+') continue;
            // aStarPathfinding fonksiyonunda:
            if (map[nextY * WIDTH + nextX] == '#') continue; // Sadece k�r�lmaz engelleri durdur

            int newCost = costSoFar[current.y * WIDTH + current.x] + 1;

            int index = nextY * WIDTH + nextX;
            if (costSoFar.find(index) == costSoFar.end() || newCost < costSoFar[index]) {
                costSoFar[index] = newCost;
                int priority = newCost + abs(goalX - nextX) + abs(goalY - nextY); // Heuristic
                frontier.push({ nextX, nextY, newCost, priority });
                cameFrom[index] = current.y * WIDTH + current.x;
            }
        }
    }

    return {}; // E�er hedefe ula��lamazsa bo� bir yol d�ner
}



std::pair<int, int> findSafeSpot(int currentX, int currentY, int botIndex) {
    for (int radius = 1; radius <= WIDTH; ++radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                int newX = currentX + dx;
                int newY = currentY + dy;

                // Harita s�n�rlar�n� ve ge�ilebilirli�i kontrol et
                if (newX >= 0 && newX < WIDTH && newY >= 0 && newY < HEIGHT &&
                    canMoveTo(newX * cellSize, newY * cellSize, botIndex) && isSafe(newX, newY)) {
                    // G�venli bir yer bulundu, en k�sa yol hesaplan�yor
                    auto path = aStarPathfinding(currentX, currentY, newX, newY);
                    if (!path.empty()) {
                        return { path[0].first, path[0].second }; // �lk ad�m
                    }
                }
            }
        }
    }

    return { currentX, currentY }; // E�er g�venli bir yer bulunamazsa oldu�u yerde kal
}




std::pair<int, int> getRandomMoveWithBlockBreaking(int currentX, int currentY, int botIndex) {
    const int dx[] = { 1, -1, 0, 0 }; // Sa�, Sol, Yukar�, A�a��
    const int dy[] = { 0, 0, 1, -1 };

    for (int i = 0; i < 4; ++i) {
        int newX = currentX + dx[i];
        int newY = currentY + dy[i];

        // E�er h�cre ge�ilebilir ise do�rudan hareket et
        if (newX >= 0 && newX < WIDTH && newY >= 0 && newY < HEIGHT &&
            canMoveTo(newX * cellSize, newY * cellSize, botIndex)) {
            return { newX, newY };
        }

        // E�er h�cre k�r�labilir blok ise, bomba koyup yol a�may� deneyelim
        if (map[newY * WIDTH + newX] == '+') {
            placeBomb(botIndex); // Bomba koy
            return { currentX, currentY }; // Bomba patlamas�n� bekle
        }
    }

    return { currentX, currentY }; // E�er hi�bir �ey yap�lamazsa oldu�u yerde kal
}



std::pair<int, int> findClosestEnemy(int botIndex) {
    int botX = playersPos[botIndex][0] / cellSize;
    int botY = playersPos[botIndex][1] / cellSize;
    int closestX = -1, closestY = -1;
    int minDistance = INT_MAX;

    for (int i = 0; i < 4; ++i) { // T�m oyuncular� ve botlar� kontrol et
        if (i == botIndex || playersPos[i][0] < 0 || playersPos[i][1] < 0) {
            continue; // Kendini veya �lm�� rakipleri es ge�
        }
        int enemyX = playersPos[i][0] / cellSize;
        int enemyY = playersPos[i][1] / cellSize;
        int distance = abs(botX - enemyX) + abs(botY - enemyY);
        if (distance < minDistance) {
            minDistance = distance;
            closestX = enemyX;
            closestY = enemyY;
        }
    }

    return { closestX, closestY }; // En yak�n rakibin pozisyonu
}


std::pair<int, int> findBestPowerUp(int botIndex) {
    int botX = playersPos[botIndex][0] / cellSize;
    int botY = playersPos[botIndex][1] / cellSize;

    int closestPowerUpX = -1, closestPowerUpY = -1;
    int minDistance = INT_MAX;

    // Haritadaki t�m power-up'lar� kontrol et
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (strchr("SRNKDT", map[y * WIDTH + x])) { // E�er h�crede g��lendirme varsa
                int distance = abs(botX - x) + abs(botY - y);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestPowerUpX = x;
                    closestPowerUpY = y;
                }
            }
        }
    }

    // E�er bir g��lendirme bulunduysa, A* ile yol bul
    if (closestPowerUpX != -1 && closestPowerUpY != -1) {
        auto path = aStarPathfinding(botX, botY, closestPowerUpX, closestPowerUpY);
        if (!path.empty()) {
            return { path[0].first, path[0].second }; // �lk ad�m
        }
    }

    return { -1, -1 }; // E�er g��lendirme yoksa veya ula��lam�yorsa
}


bool shouldPlaceBomb(int botIndex, int targetX, int targetY) {
    int botX = playersPos[botIndex][0] / cellSize;
    int botY = playersPos[botIndex][1] / cellSize;

    // E�er bot bulundu�u h�crede bomba yerle�tiremezse geri d�n
    if (map[botY * WIDTH + botX] == '#' || map[botY * WIDTH + botX] == '+') {
        return false;
    }

    // E�er hedef bir k�r�labilir blok taraf�ndan kapal�ysa bomba koy
    if (map[targetY * WIDTH + targetX] == '+') {
        return true;
    }

    return false;
}


bool isCellInDanger(int cellX, int cellY) {
    for (const auto& bomb : bombs) {
        auto affectedCells = getBombEffectArea(bomb);
        for (const auto& cell : affectedCells) {
            if (cell.first == cellX && cell.second == cellY) {
                return true;
            }
        }
    }
    return false;
}

std::pair<int, int> findBestEscapeRoute(int botIndex) {
    int botX = playersPos[botIndex][0] / cellSize;
    int botY = playersPos[botIndex][1] / cellSize;
    std::vector<std::pair<int, int>> safeSpots;

    // Haritada g�venli b�lgeleri bul
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (isSafe(x, y) && canMoveTo(x * cellSize, y * cellSize, botIndex)) {
                safeSpots.push_back({ x, y });
            }
        }
    }

    // En yak�n g�venli noktay� se�
    std::pair<int, int> bestSpot = { botX, botY };
    int minDistance = INT_MAX;
    for (auto& spot : safeSpots) {
        int distance = abs(botX - spot.first) + abs(botY - spot.second);
        if (distance < minDistance) {
            minDistance = distance;
            bestSpot = spot;
        }
    }

    return bestSpot;
}


void moveAI(int botIndex) {
    int& x = playersPos[botIndex][0];
    int& y = playersPos[botIndex][1];
    std::vector<std::pair<int, int>> path;

    while (thread_continue) {
        int cellX = x / cellSize;
        int cellY = y / cellSize;
        int replacement = 2 + multiplier[botIndex][0];

        // 1. G�venli bir b�lgeye ka��� plan� yap
        if (!isSafe(cellX, cellY)) {
            auto escapeSpot = findBestEscapeRoute(botIndex);
            path = aStarPathfinding(cellX, cellY, escapeSpot.first, escapeSpot.second);
        }
        // 2. G��lendirme ara
        else {
            auto powerUpTarget = findBestPowerUp(botIndex);
            if (powerUpTarget.first != -1 && powerUpTarget.second != -1) {
                path = aStarPathfinding(cellX, cellY, powerUpTarget.first, powerUpTarget.second);
            }
            // 3. Rakip ara
            else {
                auto enemyTarget = findClosestEnemy(botIndex);
                if (enemyTarget.first != -1 && enemyTarget.second != -1) {
                    // E�er d��man yak�nsa bomba koy
                    if (shouldPlaceBomb(botIndex, enemyTarget.first, enemyTarget.second)) {
                        placeBomb(botIndex);
                        auto escapeSpot = findBestEscapeRoute(botIndex);
                        path = aStarPathfinding(cellX, cellY, escapeSpot.first, escapeSpot.second);
                    }
                    else {
                        path = aStarPathfinding(cellX, cellY, enemyTarget.first, enemyTarget.second);
                    }
                }
                // 4. Rastgele hareket et
                else {
                    auto randomMove = getRandomMoveWithBlockBreaking(cellX, cellY, botIndex);
                    path = { randomMove };
                }
            }
        }

        // E�er yol varsa ad�m ad�m ilerle
        if (!path.empty()) {
            auto nextStep = path.front();
            path.erase(path.begin());
            int nextX = nextStep.first * cellSize + cellSize / 2;
            int nextY = nextStep.second * cellSize + cellSize / 2;

            if (x < nextX && canMoveTo(x + replacement, y, botIndex)) x += replacement;
            else if (x > nextX && canMoveTo(x - replacement, y, botIndex)) x -= replacement;
            if (y < nextY && canMoveTo(x, y + replacement, botIndex)) y += replacement;
            else if (y > nextY && canMoveTo(x, y - replacement, botIndex)) y -= replacement;
        }
        collectPowerUp((cellY * WIDTH + cellX), botIndex);

        fixBrickOverlap(cellX, cellY);
        Sleep(30);
    }
}

/***************************************INITIALS******************************************************************/

void setMulti() {
    playersPos[0][0] = cellSize * 1.5; // Oyuncu 1 ba�lang�� pozisyonu
    playersPos[0][1] = cellSize * 1.5;
    playersPos[1][0] = cellSize * 13.5; // Oyuncu 2 ba�lang�� pozisyonu
    playersPos[1][1] = cellSize * 13.5;

    playersPos[2][0] = -1; // Yapay zeka 2 ba�lang�� pozisyonu
    playersPos[2][1] = -1;
    playersPos[3][0] = -1; // Yapay zeka 3 ba�lang�� pozisyonu
    playersPos[3][1] = -1;

    if (!thread_continue) {
        initializeMap(minRatio, maxRatio);
        thread_continue = true;
        Threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer1, NULL, 0, NULL); // Oyuncu 1
        Threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer2, NULL, 0, NULL); // Oyuncu 2
        SetFocus(ICG_GetMainWindow());
    }
    else {
        thread_continue = false;
    }
}

void setSingle() {
    single = true;
    playersPos[0][0] = cellSize * 1.5; // Oyuncu ba�lang�� pozisyonu
    playersPos[0][1] = cellSize * 1.5;

    playersPos[1][0] = cellSize * 13.5; // Bot 1 ba�lang�� pozisyonu
    playersPos[1][1] = cellSize * 1.5;

    playersPos[2][0] = cellSize * 1.5; // Bot 2 ba�lang�� pozisyonu
    playersPos[2][1] = cellSize * 13.5;

    playersPos[3][0] = cellSize * 13.5; // Bot 3 ba�lang�� pozisyonu
    playersPos[3][1] = cellSize * 13.5;

    if (!thread_continue) {
        thread_continue = true;
        initializeMap(minRatio, maxRatio);

        // Oyuncu hareketi
        Threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer1, NULL, 0, NULL);

        // Bot hareketleri
        for (int i = 1; i < 4; i++) {
            Threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)moveAI, (void*)i, 0, NULL);
        }

        SetFocus(ICG_GetMainWindow());
    }
    else {
        thread_continue = false;
    }
}


void setOnline() {
    //server client
}




void StopThreads(int n) {
    thread_continue = false;  // Thread'lerin kendi d�ng�s�nden ��kmas�n� iste
    for (int i = 0; i < n; i++) {
        if (Threads[i]) {
            // E�er thread hala �al���yorsa, maksimum 2 saniye bekle
            DWORD exitCode;
            GetExitCodeThread(Threads[i], &exitCode);

            if (exitCode == STILL_ACTIVE) {
                if (WaitForSingleObject(Threads[i], 1000) == WAIT_TIMEOUT) {
                    // Thread 2 saniyede kapanmad�ysa, zorla kapat
                    TerminateThread(Threads[i], 0);
                }
            }

            CloseHandle(Threads[i]); // Kayna�� temizle
            Threads[i] = NULL;
        }
    }
}






void RestartGame() {
    StopThreads(single ? 4 : 2); // �nce �al��an t�m thread'leri durdur
    thread_continue = false; // Mevcut t�m thread'leri durdur
    Sleep(500); // Thread'lerin tamamen durmas�n� beklemek i�in k�sa bir gecikme

    initializeMap(minRatio, maxRatio); // Haritay� s�f�rla
    for (int i = 0; i < sizeof(drawnWalls); i++) {
        drawnWalls[i] = false;
    }
    if (single) {
        setSingle(); // Singleplayer modunu tekrar ba�lat
    }
    else {
        setMulti(); // Multiplayer modunu tekrar ba�lat
    }
    drawMap();
}

void TogglePause() {
    isPaused = !isPaused;

    if (isPaused) {
        thread_continue = false; // Hareket thread'lerini durdur
    }
    else {
        thread_continue = true; // Tekrar ba�lat

        // Oyuncu ve botlar� yeniden ba�lat
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer1, NULL, 0, NULL); // Oyuncu 1

        if (single) {
            for (int i = 1; i < 4; i++) { // Botlar
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)moveAI, (void*)i, 0, NULL);
            }
        }
        else {
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer2, NULL, 0, NULL); // Oyuncu 2
        }
    }
}


void WhenKeyPressed(int key) {
    if (!isMenu) {
        if (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == 'Q') {
            keyStatesPlayer1[key] = true;
        }
        else if (key == VK_UP || key == VK_LEFT || key == VK_DOWN || key == VK_RIGHT || key == VK_NUMPAD3) {
            keyStatesPlayer2[key] = true;
        }

        if (key == 'E') {
            placeBomb(0);
        }
        else if (key == VK_NUMPAD0) {
            placeBomb(1);
        }
        else if (key == 'R') {
            isWinScreen = false;
            RestartGame();
        }
        else if (key == 'P') {
            TogglePause();
        }
        else if (key == VK_ESCAPE && !isMenu) { // Ara Men�
            if(!isInControls && !isWinScreen) {
                Sleep(500);
                isMenu = true;
                isInGameMenu = true; // Oyun i�i men� aktif oldu
                fromControls = false;
                menuThread[IN_GAME_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)inGameMenu, NULL, 0, NULL);
            }
            else if (isWinScreen) {
                isWinScreen = false;
                exit(0);
            }
            else if (isInControls) {
            isInGameMenu = false;
            isMenu = false;
            Sleep(500);
            if (isCalledFromMain) {
                menuThread[MAIN_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainMenu, NULL, 0, NULL);
            }
            else {
                fromControls = true;
                menuThread[IN_GAME_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)inGameMenu, NULL, 0, NULL);
                Sleep(500);
            }
        }
        }
    }
    else { // Men� a��ksa

        if (key == 'W' || key == 'A' || key == 'S' || key == 'D' ||
            key == VK_UP || key == VK_LEFT || key == VK_DOWN || key == VK_RIGHT ||
            key == VK_SPACE || key == VK_RETURN || key == VK_SEPARATOR || key == VK_ESCAPE) {
            keyStatesPlayer1[key] = true;
        }

        if (key == VK_ESCAPE && !inMainMenu) {
            Sleep(500);

            if (isInGameMenu) { // Oyun i�i men�deyken ESC'ye bas�ld�ysa oyuna d�n
                isMenu = false;
                isInGameMenu = false;
                thread_continue = true;

                Threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer1, NULL, 0, NULL);
                if (single) {
                    for (int i = 1; i < 4; i++) {
                        Threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)moveAI, (void*)i, 0, NULL);
                    }
                }
                else {
                    Threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer2, NULL, 0, NULL);
                }
            }
            else if (isInOnlineMenu) {
                isMenu = false;
                isInOnlineMenu = false;
                Sleep(500);
                menuThread[GAME_MODE_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gameMode, NULL, 0, NULL);
            }
            else if (isInGameMode) {
                isMenu = false;
                isInGameMode = false;
                Sleep(500);
                menuThread[MAIN_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainMenu, NULL, 0, NULL);
            }
        }
    }
}


void WhenKeyReleased(int key) {
    if (!isMenu) {
        if (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == 'Q') { // W, A, S, D, Q
            keyStatesPlayer1[key] = false;
        }
        else if (key == VK_UP || key == VK_LEFT || key == VK_DOWN || key == VK_RIGHT || key == VK_NUMPAD3) { // Ok tu�lar�
            keyStatesPlayer2[key] = false;
        }
        else if (key == VK_SPACE || key == VK_RETURN || key == VK_SEPARATOR) { // VK_SEPARATOR GER� EKLEND�
            keyStatesPlayer1[key] = false;
        }
    }
    else {
        if (key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == VK_UP || key == VK_LEFT || key == VK_DOWN || key == VK_RIGHT || key == VK_SPACE || key == VK_RETURN || key == VK_SEPARATOR || key == VK_ESCAPE) {
            keyStatesPlayer1[key] = false;
        }
    }
}

void stopMenu(int currentIndex) {
    for (int i = 0; i < 7; i++) {
        if (i != currentIndex && menuThread[i] != NULL) {
            DWORD exitCode;
            GetExitCodeThread(menuThread[i], &exitCode);

            if (exitCode == STILL_ACTIVE) {
                WaitForSingleObject(menuThread[i], 1000);  // 1 saniye bekle
            }

            CloseHandle(menuThread[i]);
            menuThread[i] = NULL;
        }
    }
}



void inGameMenu() {
    stopMenu(IN_GAME_MENU_INDEX);
    if (!fromControls) {
        thread_continue = false;
        isInGameMenu = true;
    }
    isMenu = true;
    isInControls = false;
    int currentIndex = 0;
    int menuX = 275;
    int mainMenuPos[5] = { 49, 187, 325, 463, 601 };
    ICBYTES buttons[5];
    ICBYTES selectedButton[5];

    ReadImage("bg1.bmp", logo);

    ReadImage("resume.bmp", buttons[0]);
    ReadImage("restart.bmp", buttons[1]);
    ReadImage("controlsYazi.bmp", buttons[2]);
    ReadImage("main_menu.bmp", buttons[3]);
    ReadImage("exitYazi.bmp", buttons[4]);

    ReadImage("resume_selected.bmp", selectedButton[0]);
    ReadImage("restart_selected.bmp", selectedButton[1]);
    ReadImage("controlsYazi_selected.bmp", selectedButton[2]);
    ReadImage("main_menu_selected.bmp", selectedButton[3]);
    ReadImage("exitYazi_selected.bmp", selectedButton[4]);

    while (isMenu) {
        if (keyStatesPlayer1['W'] || keyStatesPlayer1[VK_UP]) {
            currentIndex = (currentIndex - 1 + 5) % 5; // Yukar� git
        }
        else if (keyStatesPlayer1['S'] || keyStatesPlayer1[VK_DOWN]) {
            currentIndex = (currentIndex + 1) % 5; // A�a�� git
        }

        if (keyStatesPlayer1[VK_SPACE] || keyStatesPlayer1[VK_RETURN]) {
            if (currentIndex == 0) { // Devam Et
                isMenu = false;
                Sleep(500);
                if (!isMenu) { // Men� kapand�ysa oyunu devam ettir
                    thread_continue = true;
                    Threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer1, NULL, 0, NULL);

                    if (single) {
                        for (int i = 1; i < 4; i++) { // Singleplayer modundaysa botlar� ba�lat
                            Threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)moveAI, (void*)i, 0, NULL);
                        }
                    }
                    else {
                        Threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movePlayer2, NULL, 0, NULL);
                    }
                }
                return;
            }
            else if (currentIndex == 1) { // Restart
                isMenu = false;
                RestartGame();
                return;
            }
            else if (currentIndex == 2) {
                isMenu = false;
                isCalledFromMain = false;
                menuThread[SETTINGS_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)controlsMenu, NULL, 0, NULL); // Controls Men�
                break;
            }
            else if (currentIndex == 3) { // Ana Men�
                isMenu = false;
                StopThreads(single ? 4 : 2);
                thread_continue = false;
                Sleep(500);
                mainMenu(); // Yeni thread a�ma, do�rudan �a��r
                return;
            }
            else { // ��k��
                exit(0);
            }
        }
        for (int i = 0; i < 5; i++) {
            if (i == currentIndex) {
                PasteNon0(selectedButton[i], menuX, mainMenuPos[i], logo);
            }
            else {
                PasteNon0(buttons[i], menuX, mainMenuPos[i], logo);
            }
            
        }

        DisplayImage(FRM1, logo);
        Sleep(100);
    }
}


void singleMenu() {
    stopMenu(SINGLE_MENU_INDEX);

}
int menuY = 275;
ICBYTES bg;
void onlineMenu() {
    stopMenu(ONLINE_MENU_INDEX);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    ICG_SetOnKeyUp(WhenKeyReleased);
    SetFocus(ICG_GetMainWindow());

    isInOnlineMenu = true;
    ICBYTES ip;
    isMenu = true;
    int currentIndex = 0;
    int menuX = 525;
    int mainMenuPos[2] = { 460, 520 };
    ICBYTES buttons[2];
    ICBYTES selectedButton[2];
    ReadImage("online_menu.bmp", logo);

    ReadImage("oda_kur.bmp", buttons[0]);
    ReadImage("katil.bmp", buttons[1]);

    ReadImage("oda_kur_selected.bmp", selectedButton[0]);
    ReadImage("katil_selected.bmp", selectedButton[1]);

    while (isMenu) {
        // Men�deki y�n tu�lar�na bas�ld���nda se�im de�i�tir
        if (keyStatesPlayer1['W'] || keyStatesPlayer1[VK_UP] || keyStatesPlayer1['D'] || keyStatesPlayer1[VK_RIGHT]) {
            currentIndex = (currentIndex + 1) % 2; // 0-1 aras�nda de�i�tir
        }
        else if (keyStatesPlayer1['S'] || keyStatesPlayer1[VK_DOWN] || keyStatesPlayer1['A'] || keyStatesPlayer1[VK_LEFT]) {
            currentIndex = (currentIndex - 1 + 2) % 2; // Geri gitmeyi sa�lar
        }

        // Men� se�im onay� i�in tu�a bas�ld���nda
        else if (keyStatesPlayer1[VK_SPACE] || keyStatesPlayer1[VK_RETURN] || keyStatesPlayer1[VK_SEPARATOR]) {
            if (currentIndex == 0) {
                isMenu = false;
                Sleep(500);
                //oda kur
                break;
            }
            else {
                //isMenu = false;
                Sleep(500);
                //kat�l
                break;
            }
        }


        for (int i = 0; i < 2; i++) {
            if (i == currentIndex) {
                Paste(selectedButton[i], menuX, mainMenuPos[i], logo);
            }
            else {
                Paste(buttons[i], menuX, mainMenuPos[i], logo);
            }
        }
        DisplayImage(FRM1, logo);
        Sleep(100); // Bu sleep, sadece g�rsel yenileme i�in kullan�lmal�.
    }
}


void gameMode() {
    stopMenu(GAME_MODE_INDEX);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    ICG_SetOnKeyUp(WhenKeyReleased);
    SetFocus(ICG_GetMainWindow());
    isInGameMode = true;
    isMenu = true;
    int currentIndex = 0;
    int mainMenuPos[3] = { 75, 300, 525 };
    ICBYTES buttons[3];
    ICBYTES selectedButton[3];
    ReadImage("bg2.bmp", logo);

    ReadImage("singleplayer.bmp", buttons[0]);
    ReadImage("multiplayer.bmp", buttons[1]);
    ReadImage("online.bmp", buttons[2]);

    ReadImage("singleplayer_selected.bmp", selectedButton[0]);
    ReadImage("multiplayer_selected.bmp", selectedButton[1]);





    while (isMenu) {
        // Men�deki y�n tu�lar�na bas�ld���nda se�im de�i�tir
        if (keyStatesPlayer1['W'] || keyStatesPlayer1[VK_UP] || keyStatesPlayer1['D'] || keyStatesPlayer1[VK_RIGHT]) {
            currentIndex = (currentIndex + 1) % 2; // 0-1 aras�nda de�i�tir
        }
        else if (keyStatesPlayer1['S'] || keyStatesPlayer1[VK_DOWN] || keyStatesPlayer1['A'] || keyStatesPlayer1[VK_LEFT]) {
            currentIndex = (currentIndex - 1 + 2) % 2; // Geri gitmeyi sa�lar
        }

        // Men� se�im onay� i�in tu�a bas�ld���nda
        else if (keyStatesPlayer1[VK_SPACE] || keyStatesPlayer1[VK_RETURN] || keyStatesPlayer1[VK_SEPARATOR]) {
            if (currentIndex == 0) {
                isMenu = false;
                Sleep(500);
                setSingle();
                break;
            }
            else if (currentIndex == 1) {
                isMenu = false;
                Sleep(500);
                setMulti();
                break;
            }
            else {
                Copy(logo, 0, 0, 750, 750, bg);
                isMenu = false;
                Sleep(500);
                menuThread[ONLINE_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)onlineMenu, NULL, 0, NULL); // Ana Men�

                //setOnline();
            }
        }


        // Se�ili butonu k�rm�z� ile vurgula
        for (int i = 0; i < 3; i++) {
            if (i == currentIndex) {
                PasteNon0(selectedButton[i], mainMenuPos[i], menuY, logo);

            }
            else {
                PasteNon0(buttons[i], mainMenuPos[i], menuY, logo);
            }
        }
        DisplayImage(FRM1, logo);
        Sleep(100);// Bu sleep, sadece g�rsel yenileme i�in kullan�lmal�.
    }
}
void controlsMenu() {
    stopMenu(SETTINGS_MENU_INDEX);
    isInControls = true;
    StopThreads(single ? 4 : 2); // �nce oyun thread'lerini tamamen kapat    
    ReadImage("controlsScreen.bmp", logo);
    isInControls = true;
    DisplayImage(FRM1, logo);
    Sleep(100);// Bu sleep, sadece g�rsel yenileme i�in kullan�lmal�.
}


void mainMenu() {
    stopMenu(MAIN_MENU_INDEX);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    ICG_SetOnKeyUp(WhenKeyReleased);
    SetFocus(ICG_GetMainWindow());
    isMenu = true;
    inMainMenu = true;
    isInControls = false;
    int currentIndex = 0;
    int buttonEdge = 200;
    int mainMenuPos[3] = { 75, 300, 525 };
    ICBYTES buttons[3];
    ICBYTES selectedButton[3];
    ICG_SetOnKeyPressed(WhenKeyPressed); // Tu� alg�lamay� aktif et
    ICG_SetOnKeyUp(WhenKeyReleased);
    ReadImage("bg1.bmp", logo);

    ReadImage("play.bmp", buttons[0]);
    ReadImage("controls.bmp", buttons[1]);
    ReadImage("exit.bmp", buttons[2]);

    ReadImage("play_selected.bmp", selectedButton[0]);
    ReadImage("controls_selected.bmp", selectedButton[1]);
    ReadImage("exit_selected.bmp", selectedButton[2]);


    // Sonsuz d�ng� ile tu�lar� s�rekli kontrol et
    while (isMenu) {
        // Men�deki y�n tu�lar�na bas�ld���nda se�im de�i�tir
        if (keyStatesPlayer1['W'] || keyStatesPlayer1[VK_UP] || keyStatesPlayer1['D'] || keyStatesPlayer1[VK_RIGHT]) {
            currentIndex = (currentIndex + 1) % 3; // 0-1 aras�nda de�i�tir
        }
        else if (keyStatesPlayer1['S'] || keyStatesPlayer1[VK_DOWN] || keyStatesPlayer1['A'] || keyStatesPlayer1[VK_LEFT]) {
            currentIndex = (currentIndex - 1 + 3) % 3; // Geri gitmeyi sa�lar
        }

        // Men� se�im onay� i�in tu�a bas�ld���nda
        else if (keyStatesPlayer1[VK_SPACE] || keyStatesPlayer1[VK_RETURN] || keyStatesPlayer1[VK_SEPARATOR]) {
            inMainMenu = false;
            if (currentIndex == 0) {
                isMenu = false;
                Sleep(500);
                menuThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gameMode, NULL, 0, NULL); // Select Men�
            }
            else if (currentIndex == 1) {
                isMenu = false;
                isCalledFromMain = true;
                menuThread[SETTINGS_MENU_INDEX] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)controlsMenu, NULL, 0, NULL); // Controls Men�
                break;
            }
            else {
                exit(0);
            }
        }

        // Se�ili butonu k�rm�z� ile vurgula
        for (int i = 0; i < 3; i++) {
            if (i == currentIndex) {
                PasteNon0(selectedButton[currentIndex], mainMenuPos[currentIndex], menuY, logo);
                //se�ilen b�y�k olcak
            }
            else {
                PasteNon0(buttons[i], mainMenuPos[i], menuY, logo);
            }
        }



        DisplayImage(FRM1, logo);
        Sleep(100); // Bu sleep, sadece g�rsel yenileme i�in kullan�lmal�.
    }
}




void CALLBACK LogoTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - logoStartTime).count();

    if (elapsed >= 3) { // 3 saniye ge�tiyse
        showLogoActive = false;
        KillTimer(hwnd, 2); // Timer'� durdur
        menuThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainMenu, NULL, 0, NULL); // Ana Men�
    }
}

// Logo g�sterme fonksiyonu
void showLogo() {
    ReadImage("intro.bmp", logo);
    DisplayImage(FRM1, logo); // Logoyu ekranda g�ster
    logoStartTime = std::chrono::steady_clock::now(); // Ba�lang�� zaman�n� kaydet
    SetTimer(ICG_GetMainWindow(), 2, 1000, LogoTimerProc); // 1 saniyede bir kontrol et
}


void loadImages() {
    ReadImage("speed.bmp", speedIco);
    ReadImage("shield.bmp", shieldPow);
    ReadImage("shieldIco.bmp", shieldIn);
    ReadImage("debuff.bmp", debuff);
    ReadImage("number_of.bmp", numOfBomb);
    ReadImage("range.bmp", range);
    ReadImage("tugla.bmp", tugla);
    ReadImage("kapan.bmp", tuzak);
    ReadImage("kapan_kurulu.bmp", tuzak_kuruldu);
    ReadImage("tuzakIco.bmp", tuzakIco);
    ReadImage("bomba.bmp", bomba);
    ReadImage("mezar.bmp", mezar);

    ReadImage("patlama_merkez.bmp", patlamaMerkez);
    ReadImage("patlama_orta_dik.bmp", patlamaOrtaDikey);
    ReadImage("patlama_orta_yatay.bmp", patlamaOrtaYatay);
    ReadImage("patlama_uc_yukari.bmp", patlamaUcYukari);
    ReadImage("patlama_uc_alt.bmp", patlamaUcAsagi);
    ReadImage("patlama_uc_sol.bmp", patlamaUcSol);
    ReadImage("patlama_uc_sag.bmp", patlamaUcSag);
}


void ICGUI_Create() {
    ICG_MWTitle("MazeBomber");
    ICG_MWSize(785, 800);

    // Ekran boyutlar�n� al
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Pencereyi ortalamak i�in pozisyon hesapla
    int posX = (screenWidth - 785) / 2;
    int posY = (screenHeight - 835) / 2;

    // Pencereyi ortala
    ICG_MWPosition(posX, posY);

    ICG_SetFont(30, 12, "");
    initializeMap(minRatio, maxRatio);
    setupTimer(ICG_GetMainWindow()); // Zamanlay�c� ba�lat�l�yor
}

void ICGUI_main() {
    CreateImage(panel, 750, 750, ICB_UINT);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    ICG_SetOnKeyUp(WhenKeyReleased);
    FRM1 = ICG_FrameMedium(0, 0, 750, 750);
    loadImages();
    showLogo();
}