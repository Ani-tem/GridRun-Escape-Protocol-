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
using namespace std;

const int N = 20;
char grid[N][N];
pair<int, int> player;
vector<pair<int, int>> enemies;
pair<int, int> safePoint;
int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
int level = 1;
int score = 0;
int health = 3;
int enemyMoveDelay = 500; // milliseconds
bool multiplayer = false;
bool paused = false;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

bool valid(int x, int y) {
    return x >= 0 && y >= 0 && x < N && y < N && grid[x][y] != '#';
}

void generateObstacles(int count) {
    while (count--) {
        int x = rand() % N;
        int y = rand() % N;
        if (grid[x][y] == '.')
            grid[x][y] = '#';
    }
}

void generatePowerups(int count) {
    while (count--) {
        int x = rand() % N;
        int y = rand() % N;
        if (grid[x][y] == '.')
            grid[x][y] = '+';
    }
}

void generateSafePoint() {
    while (true) {
        int x = rand() % N;
        int y = rand() % N;
        if (grid[x][y] == '.') {
            safePoint = {x, y};
            grid[x][y] = 'X';
            break;
        }
    }
}

void setupLevel() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            grid[i][j] = '.';

    player = {0, 0};
    grid[player.first][player.second] = 'P';

    enemies.clear();
    for (int i = 0; i < level; ++i) {
        int ex = rand() % N, ey = rand() % N;
        while (grid[ex][ey] != '.') {
            ex = rand() % N;
            ey = rand() % N;
        }
        enemies.emplace_back(ex, ey);
        grid[ex][ey] = 'E';
    }

    generateObstacles(20 + 5 * level);
    generatePowerups(3);
    generateSafePoint();
}

void printGrid() {
    clearScreen();
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j)
            cout << grid[i][j] << ' ';
        cout << '\n';
    }
    cout << "Level: " << level << "  Score: " << score << "  Health: " << health << "\n";
    cout << "Reach 'X' to advance. '+' restores health. 'E' is enemy. Press 'q' to quit, 'p' to pause/resume.\n";
}

char getInput() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

vector<pair<int, int>> dijkstraPath(pair<int, int> src, pair<int, int> target) {
    vector<vector<int>> dist(N, vector<int>(N, INT_MAX));
    vector<vector<pair<int, int>>> prev(N, vector<pair<int, int>>(N, {-1, -1}));
    priority_queue<tuple<int, int, int>, vector<tuple<int, int, int>>, greater<>> pq;

    dist[src.first][src.second] = 0;
    pq.push({0, src.first, src.second});

    while (!pq.empty()) {
        auto [d, x, y] = pq.top(); pq.pop();
        if (make_pair(x, y) == target) break;

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i], ny = y + dy[i];
            if (valid(nx, ny) && dist[nx][ny] > d + 1) {
                dist[nx][ny] = d + 1;
                prev[nx][ny] = {x, y};
                pq.push({dist[nx][ny], nx, ny});
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

void movePlayer(char dir) {
    int nx = player.first, ny = player.second;
    if (dir == 'w') nx--;
    if (dir == 's') nx++;
    if (dir == 'a') ny--;
    if (dir == 'd') ny++;
    if (valid(nx, ny)) {
        if (grid[nx][ny] == '+') {
            health = min(health + 1, 5);
        }
        if (grid[nx][ny] == 'X') {
            level++;
            score += 10 * level;
            enemyMoveDelay = max(100, enemyMoveDelay - 50);
            setupLevel();
            return;
        }
        grid[player.first][player.second] = '.';
        player = {nx, ny};
        grid[nx][ny] = 'P';
    }
}

void moveEnemies() {
    for (auto& enemy : enemies) {
        grid[enemy.first][enemy.second] = '.';
        auto path = dijkstraPath(enemy, player);
        if (!path.empty()) enemy = path[0];
        if (enemy == player) {
            health--;
            if (health == 0) {
                clearScreen();
                cout << "\U0001F480 Game Over: Enemy caught you!\nFinal Score: " << score << "\n";
                exit(0);
            }
            grid[player.first][player.second] = '.';
            player = {0, 0};
            grid[player.first][player.second] = 'P';
        }
        grid[enemy.first][enemy.second] = 'E';
    }
}

void mainMenu() {
    clearScreen();
    cout << "===== Gridrun Escape Protocol =====\n";
    cout << "1. Single Player\n";
    cout << "2. Multiplayer (coming soon)\n";
    cout << "Choose option: ";
    char choice;
    cin >> choice;
    if (choice == '1') multiplayer = false;
    else if (choice == '2') multiplayer = true;
    else exit(0);
}

int main() {
    srand(time(0));
    mainMenu();
    setupLevel();
    printGrid();

    while (true) {
        if (player == safePoint) continue;
        char input = getInput();
        if (input == 'q') {
            cout << "\nGame exited. Final Score: " << score << "\n";
            break;
        }
        if (input == 'p') {
            paused = !paused;
            if (paused) {
                cout << "\nGame paused. Press 'p' again to resume.\n";
                continue;
            }
        }
        if (!paused) {
            movePlayer(input);
            moveEnemies();
            printGrid();
            score++;
            std::this_thread::sleep_for(std::chrono::milliseconds(enemyMoveDelay));
        }
    }

    return 0;
}