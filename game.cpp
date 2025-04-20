#include <iostream>
#include <vector>
#include <queue>
#include <tuple>
#include <climits>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <fstream>
#include <map>
#include <random>
#include <ncurses.h>
#include <ctime>
using namespace std;

const int N = 20;
char grid[N][N];
char terrainGrid[N][N]; // Stores underlying terrain
pair<int, int> player1;
pair<int, int> player2;
vector<tuple<int, int, int>> enemies; // x, y, enemy type
pair<int, int> safePoint;
int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1}; // Adding diagonals
int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
int level = 1;
int score = 0;
int health1 = 3, health2 = 3;
int armor1 = 0, armor2 = 0;
int weapons1 = 0, weapons2 = 0;
int enemyMoveDelay = 100;
bool multiplayer = false;
bool paused = false;
int gameTime = 0;
mt19937 rng(time(nullptr));

// Colors for ncurses
#define COLOR_PLAYER1 1
#define COLOR_PLAYER2 2
#define COLOR_ENEMY 3
#define COLOR_POWERUP 4
#define COLOR_SAFE 5
#define COLOR_WALL 6
#define COLOR_WATER 7
#define COLOR_LAVA 8
#define COLOR_WEAPON 9
#define COLOR_ARMOR 10
#define COLOR_TRAP 11
#define COLOR_BOSS 12

// Enemy types
enum EnemyType {
    NORMAL = 0,     // Basic enemy, follows directly
    WANDERER = 1,   // Moves randomly sometimes
    HUNTER = 2,     // Faster and more aggressive
    GHOST = 3,      // Can move through walls
    BOSS = 4        // Stronger, requires multiple hits
};

// Power-up types
enum PowerupType {
    HEALTH = 0,
    SPEED = 1,
    INVINCIBILITY = 2,
    WEAPON = 3,
    ARMOR = 4
};

map<char, string> symbolDescriptions = {
    {'1', "Player 1"},
    {'2', "Player 2"},
    {'E', "Basic Enemy"},
    {'W', "Wanderer Enemy"},
    {'H', "Hunter Enemy"},
    {'G', "Ghost Enemy"},
    {'B', "Boss Enemy"},
    {'#', "Wall"},
    {'~', "Water (Slows Movement)"},
    {'%', "Lava (Damages Health)"},
    {'X', "Safe Point/Exit"},
    {'+', "Health Power-up"},
    {'S', "Speed Power-up"},
    {'I', "Invincibility Power-up"},
    {'>', "Weapon Power-up"},
    {'A', "Armor Power-up"},
    {'T', "Trap"}
};

// Player status effects
int player1SpeedBoost = 0;
int player2SpeedBoost = 0;
int player1Invincibility = 0;
int player2Invincibility = 0;

void initNCurses() {
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    // Initialize color pairs
    init_pair(COLOR_PLAYER1, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PLAYER2, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_ENEMY, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_POWERUP, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_SAFE, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_WALL, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_WATER, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_LAVA, COLOR_YELLOW, COLOR_RED);
    init_pair(COLOR_WEAPON, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_ARMOR, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_TRAP, COLOR_BLACK, COLOR_RED);
    init_pair(COLOR_BOSS, COLOR_RED, COLOR_YELLOW);
}

void endNCurses() {
    endwin();
}

void clearScreen() {
    clear();
}

// Adds perlin-like noise for terrain generation
vector<vector<float>> generateSimpleNoise(int width, int height) {
    vector<vector<float>> noise(width, vector<float>(height, 0));
    uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate random values
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            noise[i][j] = dist(rng);
        }
    }
    
    // Simple smoothing
    vector<vector<float>> smoothed(width, vector<float>(height, 0));
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            float sum = 0;
            int count = 0;
            
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    
                    if (ni >= 0 && ni < width && nj >= 0 && nj < height) {
                        sum += noise[ni][nj];
                        count++;
                    }
                }
            }
            
            smoothed[i][j] = sum / count;
        }
    }
    
    return smoothed;
}

bool valid(int x, int y, bool isGhost = false) {
    return x >= 0 && y >= 0 && x < N && y < N && (isGhost || grid[x][y] != '#');
}

void generateTerrain() {
    vector<vector<float>> noise = generateSimpleNoise(N, N);
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (noise[i][j] < 0.2) {
                terrainGrid[i][j] = '~'; // Water
            } else if (noise[i][j] > 0.85) {
                terrainGrid[i][j] = '%'; // Lava
            } else {
                terrainGrid[i][j] = '.'; // Normal ground
            }
        }
    }
}

void generateObstacles(int count) {
    uniform_int_distribution<int> positionDist(0, N-1);
    
    int attempts = 0;
    while (count > 0 && attempts < 1000) {
        attempts++;
        int x = positionDist(rng);
        int y = positionDist(rng);
        
        // Check if this is a valid place for an obstacle
        if (grid[x][y] == '.' && 
            (abs(x - player1.first) > 3 || abs(y - player1.second) > 3) &&
            (abs(x - safePoint.first) > 3 || abs(y - safePoint.second) > 3)) {
            grid[x][y] = '#';
            count--;
        }
    }
}

void generatePowerups() {
    uniform_int_distribution<int> positionDist(0, N-1);
    uniform_int_distribution<int> typeDist(0, 4); // Different powerup types
    
    // Generate various powerups
    int powerupCount = 3 + level / 2; // More powerups in higher levels
    int attempts = 0;
    
    while (powerupCount > 0 && attempts < 1000) {
        attempts++;
        int x = positionDist(rng);
        int y = positionDist(rng);
        
        if (grid[x][y] == '.') {
            PowerupType type = static_cast<PowerupType>(typeDist(rng));
            
            switch(type) {
                case HEALTH:
                    grid[x][y] = '+';
                    break;
                case SPEED:
                    grid[x][y] = 'S';
                    break;
                case INVINCIBILITY:
                    grid[x][y] = 'I';
                    break;
                case WEAPON:
                    grid[x][y] = '>';
                    break;
                case ARMOR:
                    grid[x][y] = 'A';
                    break;
            }
            powerupCount--;
        }
    }
    
    // Add some traps
    int trapCount = level;
    attempts = 0;
    
    while (trapCount > 0 && attempts < 500) {
        attempts++;
        int x = positionDist(rng);
        int y = positionDist(rng);
        
        if (grid[x][y] == '.' && 
           (abs(x - player1.first) > 5 || abs(y - player1.second) > 5) &&
           (abs(x - safePoint.first) > 5 || abs(y - safePoint.second) > 5)) {
            grid[x][y] = 'T';
            trapCount--;
        }
    }
}

void generateSafePoint() {
    uniform_int_distribution<int> positionDist(0, N-1);
    
    while (true) {
        int x = positionDist(rng);
        int y = positionDist(rng);
        
        // Place safe point far from players
        if (grid[x][y] == '.' && 
           (abs(x - player1.first) > N/2 || abs(y - player1.second) > N/2) &&
           (!multiplayer || abs(x - player2.first) > N/2 || abs(y - player2.second) > N/2)) {
            safePoint = {x, y};
            grid[x][y] = 'X';
            break;
        }
    }
}

void setupLevel() {
    // Reset the grid
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            grid[i][j] = '.';
        }
    }
    
    // Generate terrain first
    generateTerrain();
    
    // Setup players
    player1 = {0, 0};
    grid[player1.first][player1.second] = '1';
    
    if (multiplayer) {
        player2 = {0, N - 1};
        grid[player2.first][player2.second] = '2';
    }
    
    // Add safe point before adding enemies and obstacles
    generateSafePoint();
    
    // Add obstacles
    generateObstacles(15 + 5 * level);
    
    // Add enemies with different types
    enemies.clear();
    uniform_int_distribution<int> positionDist(0, N-1);
    uniform_int_distribution<int> typeDist(0, 100);
    
    // Calculate enemy distribution based on level
    int normalEnemies = level;
    int wandererEnemies = level / 2;
    int hunterEnemies = level / 3;
    int ghostEnemies = level / 4;
    int bossEnemies = (level % 5 == 0) ? 1 : 0; // Boss every 5 levels
    
    // Add normal enemies
    for (int i = 0; i < normalEnemies; ++i) {
        int attempts = 0;
        while (attempts < 100) {
            int ex = positionDist(rng), ey = positionDist(rng);
            if (grid[ex][ey] == '.' && 
               (abs(ex - player1.first) > 5 || abs(ey - player1.second) > 5) &&
               (!multiplayer || abs(ex - player2.first) > 5 || abs(ey - player2.second) > 5)) {
                enemies.emplace_back(ex, ey, NORMAL);
                grid[ex][ey] = 'E';
                break;
            }
            attempts++;
        }
    }
    
    // Add wanderer enemies
    for (int i = 0; i < wandererEnemies; ++i) {
        int attempts = 0;
        while (attempts < 100) {
            int ex = positionDist(rng), ey = positionDist(rng);
            if (grid[ex][ey] == '.' && 
               (abs(ex - player1.first) > 5 || abs(ey - player1.second) > 5) &&
               (!multiplayer || abs(ex - player2.first) > 5 || abs(ey - player2.second) > 5)) {
                enemies.emplace_back(ex, ey, WANDERER);
                grid[ex][ey] = 'W';
                break;
            }
            attempts++;
        }
    }
    
    // Add hunter enemies
    for (int i = 0; i < hunterEnemies; ++i) {
        int attempts = 0;
        while (attempts < 100) {
            int ex = positionDist(rng), ey = positionDist(rng);
            if (grid[ex][ey] == '.' && 
               (abs(ex - player1.first) > 7 || abs(ey - player1.second) > 7) &&
               (!multiplayer || abs(ex - player2.first) > 7 || abs(ey - player2.second) > 7)) {
                enemies.emplace_back(ex, ey, HUNTER);
                grid[ex][ey] = 'H';
                break;
            }
            attempts++;
        }
    }
    
    // Add ghost enemies
    for (int i = 0; i < ghostEnemies; ++i) {
        int attempts = 0;
        while (attempts < 100) {
            int ex = positionDist(rng), ey = positionDist(rng);
            if (grid[ex][ey] == '.' && 
               (abs(ex - player1.first) > 7 || abs(ey - player1.second) > 7) &&
               (!multiplayer || abs(ex - player2.first) > 7 || abs(ey - player2.second) > 7)) {
                enemies.emplace_back(ex, ey, GHOST);
                grid[ex][ey] = 'G';
                break;
            }
            attempts++;
        }
    }
    
    // Add boss enemy
    if (bossEnemies > 0) {
        int attempts = 0;
        while (attempts < 100) {
            int ex = positionDist(rng), ey = positionDist(rng);
            if (grid[ex][ey] == '.' && 
               (abs(ex - player1.first) > 10 || abs(ey - player1.second) > 10) &&
               (!multiplayer || abs(ex - player2.first) > 10 || abs(ey - player2.second) > 10)) {
                enemies.emplace_back(ex, ey, BOSS);
                grid[ex][ey] = 'B';
                break;
            }
            attempts++;
        }
    }
    
    // Add powerups
    generatePowerups();
    
    // Reset player status effects
    player1SpeedBoost = 0;
    player2SpeedBoost = 0;
    player1Invincibility = 0;
    player2Invincibility = 0;
}

void printGrid() {
    clearScreen();
    
    // Draw border
    for (int j = 0; j < N + 2; j++) {
        mvaddch(0, j, '*');
        mvaddch(N + 1, j, '*');
    }
    for (int i = 0; i < N + 2; i++) {
        mvaddch(i, 0, '*');
        mvaddch(i, N + 1, '*');
    }
    
    // Draw grid
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            char displayChar = grid[i][j];
            if (displayChar == '.') {
                displayChar = terrainGrid[i][j];
            }
            
            // Set color based on character
            switch (displayChar) {
                case '1':
                    attron(COLOR_PAIR(COLOR_PLAYER1));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_PLAYER1));
                    break;
                case '2':
                    attron(COLOR_PAIR(COLOR_PLAYER2));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_PLAYER2));
                    break;
                case 'E':
                    attron(COLOR_PAIR(COLOR_ENEMY));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_ENEMY));
                    break;
                case 'W':
                    attron(COLOR_PAIR(COLOR_ENEMY));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_ENEMY));
                    break;
                case 'H':
                    attron(COLOR_PAIR(COLOR_ENEMY));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_ENEMY));
                    break;
                case 'G':
                    attron(COLOR_PAIR(COLOR_ENEMY) | A_BLINK);
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_ENEMY) | A_BLINK);
                    break;
                case 'B':
                    attron(COLOR_PAIR(COLOR_BOSS));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_BOSS));
                    break;
                case '#':
                    attron(COLOR_PAIR(COLOR_WALL));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_WALL));
                    break;
                case '~':
                    attron(COLOR_PAIR(COLOR_WATER));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_WATER));
                    break;
                case '%':
                    attron(COLOR_PAIR(COLOR_LAVA));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_LAVA));
                    break;
                case 'X':
                    attron(COLOR_PAIR(COLOR_SAFE));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_SAFE));
                    break;
                case '+':
                case 'S':
                case 'I':
                    attron(COLOR_PAIR(COLOR_POWERUP));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_POWERUP));
                    break;
                case '>':
                    attron(COLOR_PAIR(COLOR_WEAPON));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_WEAPON));
                    break;
                case 'A':
                    attron(COLOR_PAIR(COLOR_ARMOR));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_ARMOR));
                    break;
                case 'T':
                    attron(COLOR_PAIR(COLOR_TRAP));
                    mvaddch(i+1, j+1, displayChar);
                    attroff(COLOR_PAIR(COLOR_TRAP));
                    break;
                default:
                    mvaddch(i+1, j+1, displayChar);
            }
        }
    }
    
    // Display status
    mvprintw(N+2, 1, "Level: %d  Score: %d  Time: %d", level, score, gameTime);
    mvprintw(N+3, 1, "P1: HP:%d ARM:%d WPN:%d", health1, armor1, weapons1);
    
    if (player1SpeedBoost > 0)
        mvprintw(N+3, 25, "SPEED:%d ", player1SpeedBoost);
    if (player1Invincibility > 0)
        mvprintw(N+3, 35, "INVULN:%d ", player1Invincibility);
    
    if (multiplayer) {
        mvprintw(N+4, 1, "P2: HP:%d ARM:%d WPN:%d", health2, armor2, weapons2);
        if (player2SpeedBoost > 0)
            mvprintw(N+4, 25, "SPEED:%d ", player2SpeedBoost);
        if (player2Invincibility > 0)
            mvprintw(N+4, 35, "INVULN:%d ", player2Invincibility);
    }
    
    // Display legend
    int legendY = N+5;
    mvprintw(legendY, 1, "Legend:");
    int col = 0;
    for (const auto& [symbol, desc] : symbolDescriptions) {
        if (col % 3 == 0 && col > 0) {
            legendY++;
        }
        mvprintw(legendY + col/3, 10 + (col%3)*20, "%c: %s", symbol, desc.c_str());
        col++;
    }
    
    // Controls
    mvprintw(legendY + col/3 + 1, 1, "Controls: P1: [wasd] + [f] attack | P2: [ijkl] + [;] attack | [p] pause | [q] quit | [m] save");
    
    refresh();
}

int getInput() {
    return getch();
}

vector<pair<int, int>> dijkstraPath(pair<int, int> src, pair<int, int> target, bool isGhost = false) {
    vector<vector<int>> dist(N, vector<int>(N, INT_MAX));
    vector<vector<pair<int, int>>> prev(N, vector<pair<int, int>>(N, {-1, -1}));
    priority_queue<tuple<int, int, int>, vector<tuple<int, int, int>>, greater<>> pq;

    dist[src.first][src.second] = 0;
    pq.push({0, src.first, src.second});

    while (!pq.empty()) {
        auto [d, x, y] = pq.top(); pq.pop();
        if (make_pair(x, y) == target) break;

        for (int i = 0; i < 4; ++i) { // Only use cardinal directions for pathfinding
            int nx = x + dx[i], ny = y + dy[i];
            if (nx >= 0 && ny >= 0 && nx < N && ny < N) {
                // Ghost enemies can move through walls
                if (isGhost || grid[nx][ny] != '#') {
                    // Calculate movement cost based on terrain
                    int cost = 1;
                    if (terrainGrid[nx][ny] == '~') cost = 3; // Water slows down movement
                    if (terrainGrid[nx][ny] == '%') cost = 2; // Lava is dangerous but can be traversed
                    
                    if (dist[nx][ny] > d + cost) {
                        dist[nx][ny] = d + cost;
                        prev[nx][ny] = {x, y};
                        pq.push({dist[nx][ny], nx, ny});
                    }
                }
            }
        }
    }

    vector<pair<int, int>> path;
    pair<int, int> cur = target;
    while (cur != src && prev[cur.first][cur.second] != make_pair(-1, -1)) {
        path.push_back(cur);
        cur = prev[cur.first][cur.second];
    }
    reverse(path.begin(), path.end());
    return path;
}

void checkTerrainEffects(pair<int, int> &player, int &health) {
    char terrain = terrainGrid[player.first][player.second];
    
    if (terrain == '%') { // Lava damages health
        health = max(0, health - 1);
    }
}

bool attackEnemy(pair<int, int> player, int weaponCount) {
    if (weaponCount <= 0) return false;
    
    // Check all adjacent positions for enemies
    for (int i = 0; i < 8; ++i) { // Including diagonals
        int nx = player.first + dx[i];
        int ny = player.second + dy[i];
        
        if (nx >= 0 && ny >= 0 && nx < N && ny < N) {
            char cell = grid[nx][ny];
            if (cell == 'E' || cell == 'W' || cell == 'H' || cell == 'G' || cell == 'B') {
                // Find this enemy in our vector
                for (size_t j = 0; j < enemies.size(); ++j) {
                    if (get<0>(enemies[j]) == nx && get<1>(enemies[j]) == ny) {
                        // If it's a boss, it requires multiple hits
                        if (get<2>(enemies[j]) == BOSS) {
                            // Deal damage but don't remove yet
                            // We'll simulate 3 hits to kill a boss
                            static map<pair<int, int>, int> bossHealth;
                            if (bossHealth.find({nx, ny}) == bossHealth.end()) {
                                bossHealth[{nx, ny}] = 3;
                            }
                            
                            bossHealth[{nx, ny}]--;
                            if (bossHealth[{nx, ny}] <= 0) {
                                grid[nx][ny] = '.';
                                enemies.erase(enemies.begin() + j);
                                score += 50; // Bonus for killing a boss
                            }
                        } else {
                            // Regular enemies die in one hit
                            grid[nx][ny] = '.';
                            enemies.erase(enemies.begin() + j);
                            score += 10; // Bonus for killing an enemy
                        }
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void movePlayer(pair<int, int> &player, int &health, int &armor, int &weapons, 
                int &speedBoost, int &invincibility, int input, char symbol) {
    int nx = player.first, ny = player.second;
    bool isMove = false;
    
    // Movement
    if (input == 'w' || input == 'i' || input == KEY_UP) { nx--; isMove = true; }
    if (input == 's' || input == 'k' || input == KEY_DOWN) { nx++; isMove = true; }
    if (input == 'a' || input == 'j' || input == KEY_LEFT) { ny--; isMove = true; }
    if (input == 'd' || input == 'l' || input == KEY_RIGHT) { ny++; isMove = true; }
    
    // Attack
    if (input == 'f' || input == ';' || input == ' ') {
        bool attackSuccess = attackEnemy(player, weapons);
        if (attackSuccess) {
            weapons = max(0, weapons - 1); // Use up weapon charge
        }
        return;
    }
    
    if (!isMove) return;
    
    if (nx >= 0 && ny >= 0 && nx < N && ny < N && (grid[nx][ny] != '#')) {
        char target = grid[nx][ny];
        
        // Check for special spaces
        if (target == '+') health = min(health + 1, 5);
        else if (target == 'S') speedBoost += 10;
        else if (target == 'I') invincibility += 10;
        else if (target == '>') weapons = min(weapons + 3, 10);
        else if (target == 'A') armor = min(armor + 1, 3);
        else if (target == 'T') {
            if (invincibility <= 0) {
                health = max(0, health - (armor > 0 ? 1 : 2));
                if (armor > 0) armor--;
            }
        }
        else if (target == 'X') {
            level++;
            score += 100 * level;
            enemyMoveDelay = max(50, enemyMoveDelay - 5);
            setupLevel();
            return;
        }
        else if (target == 'E' || target == 'W' || target == 'H' || target == 'G' || target == 'B') {
            // Hit by enemy
            if (invincibility <= 0) {
                if (armor > 0) {
                    armor--;
                } else {
                    health--;
                }
                
                if (health <= 0) {
                    endNCurses();
                    cout << "\nGame Over: Player " << symbol << " was caught!\n";
                    cout << "Final Score: " << score << "\n";
                    cout << "Level Reached: " << level << "\n";
                    cout << "Time Survived: " << gameTime << " seconds\n";
                    exit(0);
                }
                
                // Reset position
                grid[player.first][player.second] = '.';
                player = (symbol == '1') ? make_pair(0, 0) : make_pair(0, N-1);
                grid[player.first][player.second] = symbol;
                return;
            }
        }
        
        // Make the move
        grid[player.first][player.second] = '.';
        player = {nx, ny};
        grid[nx][ny] = symbol;
        
        // Apply terrain effects
        checkTerrainEffects(player, health);
    }
}

void moveEnemies() {
    uniform_int_distribution<int> randomDirDist(0, 3); // For random movement
    uniform_int_distribution<int> randomMoveDist(0, 100); // For wanderer randomness
    
    for (size_t i = 0; i < enemies.size(); ++i) {
        auto& [ex, ey, type] = enemies[i];
        char enemySymbol;
        
        // Get the correct symbol for this enemy type
        switch (type) {
            case NORMAL: enemySymbol = 'E'; break;
            case WANDERER: enemySymbol = 'W'; break;
            case HUNTER: enemySymbol = 'H'; break;
            case GHOST: enemySymbol = 'G'; break;
            case BOSS: enemySymbol = 'B'; break;
            default: enemySymbol = 'E';
        }
        
        // Skip if it's dead
        if (grid[ex][ey] != enemySymbol) continue;
        
        // Clear current position
        grid[ex][ey] = '.';
        
        int nx = ex, ny = ey;
        
        // Different movement patterns based on enemy type
        if (type == WANDERER && randomMoveDist(rng) < 30) {
            // 30% chance to move randomly
            int direction = randomDirDist(rng);
            nx = ex + dx[direction];
            ny = ey + dy[direction];
        } else if (type == GHOST) {
            // Ghost follows shortest path, ignoring walls
            auto path = dijkstraPath({ex, ey}, multiplayer ? 
                (health1 <= health2 ? player1 : player2) : player1, true);
            
            if (!path.empty()) {
                nx = path[0].first;
                ny = path[0].second;
            }
        } else if (type == HUNTER) {
            // Hunter moves twice as fast (50% chance for another move)
            auto path = dijkstraPath({ex, ey}, multiplayer ? 
                (health1 <= health2 ? player1 : player2) : player1);
            
            if (!path.empty()) {
                nx = path[0].first;
                ny = path[0].second;
            }
        } else {
            // Normal enemy pathfinding
            auto path = dijkstraPath({ex, ey}, multiplayer ? 
                (health1 <= health2 ? player1 : player2) : player1);
            
            if (!path.empty()) {
                nx = path[0].first;
                ny = path[0].second;
            }
        }
        
        // Check if valid move (Ghost can move through walls)
        if (valid(nx, ny, type == GHOST)) {
            // Check if destination has a player
            if ((nx == player1.first && ny == player1.second)) {
                if (player1Invincibility <= 0) {
                    if (armor1 > 0) {
                        armor1--;
                    } else {
                        health1--;
                    }
                    
                    if (health1 <= 0) {
                        endNCurses();
                        cout << "\nGame Over: Player 1 was caught!\n";
                        cout << "Final Score: " << score << "\n";
                        cout << "Level Reached: " << level << "\n";
                        cout << "Time Survived: " << gameTime << " seconds\n";
                        exit(0);
                    }
                    
                    // Reset position after being hit
                    grid[player1.first][player1.second] = '.';
                    player1 = {0, 0};
                    grid[player1.first][player1.second] = '1';
                }
                
                // Enemy stays in place after hitting player
                enemies[i] = {ex, ey, type};
                grid[ex][ey] = enemySymbol;
            } else if (multiplayer && nx == player2.first && ny == player2.second) {
                if (player2Invincibility <= 0) {
                    if (armor2 > 0) {
                        armor2--;
                    } else {
                        health2--;
                    }
                    
                    if (health2 <= 0) {
                        endNCurses();
                        cout << "\nGame Over: Player 2 was caught!\n";
                        cout << "Final Score: " << score << "\n";
                        cout << "Level Reached: " << level << "\n";
                        cout << "Time Survived: " << gameTime << " seconds\n";
                        exit(0);
                    }
                    
                    // Reset position after being hit
                    grid[player2.first][player2.second] = '.';
                    player2 = {0, N-1};
                    grid[player2.first][player2.second] = '2';
                }
                
                // Enemy stays in place after hitting player
                enemies[i] = {ex, ey, type};
                grid[ex][ey] = enemySymbol;
            } else if (grid[nx][ny] == '.') {
                // Move enemy
                enemies[i] = {nx, ny, type};
                grid[nx][ny] = enemySymbol;
            } else {
                // Blocked by another enemy or obstacle, stay in place
                enemies[i] = {ex, ey, type};
                grid[ex][ey] = enemySymbol;
            }
        } else {
            // Invalid move, stay in place
            enemies[i] = {ex, ey, type};
            grid[ex][ey] = enemySymbol;
        }
        
        // Hunter gets a second move
        if (type == HUNTER && randomMoveDist(rng) < 50) {
            // Recursively give this hunter another move
            i--; // Process this enemy again
        }
    }
}

void updateStatusEffects() {
    if (player1SpeedBoost > 0) player1SpeedBoost--;
    if (player2SpeedBoost > 0) player2SpeedBoost--;
    if (player1Invincibility > 0) player1Invincibility--;
    if (player2Invincibility > 0) player2Invincibility--;
}

void saveGame(const string& filename) {
    ofstream file(filename);
    if (!file) {
        mvprintw(N+10, 1, "Failed to save game!");
        refresh();
        return;
    }
    
    // Save game state
    file << level << " " << score << " " << gameTime << endl;
    file << health1 << " " << armor1 << " " << weapons1 << " " << player1SpeedBoost << " " << player1Invincibility << endl;
    file << health2 << " " << armor2 << " " << weapons2 << " " << player2SpeedBoost << " " << player2Invincibility << endl;
    file << player1.first << " " << player1.second << endl;
    file << player2.first << " " << player2.second << endl;
    file << safePoint.first << " " << safePoint.second << endl;
    file << multiplayer << endl;
    
    // Save grid
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            file << grid[i][j];
        }
        file << endl;
    }
    
    // Save terrain grid
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            file << terrainGrid[i][j];
        }
        file << endl;
    }
    
    // Save enemies
    file << enemies.size() << endl;
    for (const auto& enemy : enemies) {
        file << get<0>(enemy) << " " << get<1>(enemy) << " " << get<2>(enemy) << endl;
    }
    
    file.close();
    mvprintw(N+10, 1, "Game saved successfully!");
    refresh();
}

bool loadGame(const string& filename) {
    ifstream file(filename);
    if (!file) {
        return false;
    }
    
    // Load game state
    file >> level >> score >> gameTime;
    file >> health1 >> armor1 >> weapons1 >> player1SpeedBoost >> player1Invincibility;
    file >> health2 >> armor2 >> weapons2 >> player2SpeedBoost >> player2Invincibility;
    file >> player1.first >> player1.second;
    file >> player2.first >> player2.second;
    file >> safePoint.first >> safePoint.second;
    file >> multiplayer;
    
    // Consume newline
    string line;
    getline(file, line);
    
    // Load grid
    for (int i = 0; i < N; ++i) {
        getline(file, line);
        for (int j = 0; j < N && j < line.size(); ++j) {
            grid[i][j] = line[j];
        }
    }
    
    // Load terrain grid
    for (int i = 0; i < N; ++i) {
        getline(file, line);
        for (int j = 0; j < N && j < line.size(); ++j) {
            terrainGrid[i][j] = line[j];
        }
    }
    
    // Load enemies
    int enemyCount;
    file >> enemyCount;
    enemies.clear();
    for (int i = 0; i < enemyCount; ++i) {
        int x, y, type;
        file >> x >> y >> type;
        enemies.emplace_back(x, y, static_cast<EnemyType>(type));
    }
    
    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    bool loadFromSave = false;
    string saveFile = "game_save.txt";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--load") {
            loadFromSave = true;
        } else if (arg == "--save" && i + 1 < argc) {
            saveFile = argv[++i];
        } else if (arg == "--multiplayer") {
            multiplayer = true;
        }
    }
    
    initNCurses();
    
    if (loadFromSave) {
        if (!loadGame(saveFile)) {
            endNCurses();
            cout << "Failed to load game from " << saveFile << endl;
            return 1;
        }
    } else {
        setupLevel();
    }
    
    bool running = true;
    int lastEnemyMove = 0;
    int gameStartTime = time(nullptr);
    
    while (running && health1 > 0 && (!multiplayer || health2 > 0)) {
        printGrid();
        
        // Update game time
        gameTime = time(nullptr) - gameStartTime;
        
        // Process input
        int ch = getInput();
        if (ch == 'q' || ch == 'Q') {
            running = false;
        } else if (ch == 'p' || ch == 'P') {
            paused = !paused;
            if (paused) {
                mvprintw(N/2, N/2-4, "PAUSED");
                refresh();
                while (getInput() != 'p' && getInput() != 'P') {
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
                paused = false;
            }
        } else if (ch == 'm' || ch == 'M') {
            saveGame(saveFile);
        } else {
            // Handle player movement
            if ((player1SpeedBoost > 0 || lastEnemyMove % 2 == 0) && 
                (ch == 'w' || ch == 's' || ch == 'a' || ch == 'd' || ch == 'f')) {
                movePlayer(player1, health1, armor1, weapons1, player1SpeedBoost, player1Invincibility, ch, '1');
            }
            
            if (multiplayer && (player2SpeedBoost > 0 || lastEnemyMove % 2 == 0) && 
                (ch == 'i' || ch == 'k' || ch == 'j' || ch == 'l' || ch == ';')) {
                movePlayer(player2, health2, armor2, weapons2, player2SpeedBoost, player2Invincibility, ch, '2');
            }
            
            // Move enemies every few frames
            if (lastEnemyMove >= enemyMoveDelay) {
                moveEnemies();
                updateStatusEffects();
                lastEnemyMove = 0;
            } else {
                lastEnemyMove++;
            }
        }
        
        // Check if all enemies are defeated
        if (enemies.empty()) {
            level++;
            score += 100 * level;
            enemyMoveDelay = max(50, enemyMoveDelay - 5);
            setupLevel();
        }
        
        // Slow down the game loop
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    endNCurses();
    
    cout << "\nGame Over!\n";
    cout << "Final Score: " << score << "\n";
    cout << "Level Reached: " << level << "\n";
    cout << "Time Survived: " << gameTime << " seconds\n";
    
    return 0;
}
