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
pair<int, int> player1;
pair<int, int> player2;
vector<pair<int, int>> enemies;
pair<int, int> safePoint;
int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
int level = 1;
int score = 0;
int health1 = 3, health2 = 3;
int enemyMoveDelay = 10;
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

    player1 = {0, 0};
    grid[player1.first][player1.second] = '1';

    if (multiplayer) {
        player2 = {0, N - 1};
        grid[player2.first][player2.second] = '2';
    }

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
    cout << "Level: " << level << "  Score: " << score 
         << "  P1 Health: " << health1 
         << "  P2 Health: " << (multiplayer ? to_string(health2) : "N/A") << "\n";
    cout << "Reach 'X' to advance. '+' restores health. 'E' is enemy. 'q' to quit, 'p' to pause/resume.\n";
    if (multiplayer) cout << "P1: w a s d | P2: i j k l\n";
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

void movePlayer(pair<int, int> &player, int &health, char dir, char symbol) {
    int nx = player.first, ny = player.second;
    if (dir == 'w' || dir == 'i') nx--;
    if (dir == 's' || dir == 'k') nx++;
    if (dir == 'a' || dir == 'j') ny--;
    if (dir == 'd' || dir == 'l') ny++;

    if (valid(nx, ny)) {
        if (grid[nx][ny] == '+') health = min(health + 1, 5);
        if (grid[nx][ny] == 'X') {
            level++;
            score += 10 * level;
            enemyMoveDelay = max(100, enemyMoveDelay - 50);
            setupLevel();
            return;
        }
        grid[player.first][player.second] = '.';
        player = {nx, ny};
        grid[nx][ny] = symbol;
    }
}

void moveEnemies() {
    for (auto& enemy : enemies) {
        grid[enemy.first][enemy.second] = '.';

        pair<int, int> target = (dijkstraPath(enemy, player1).size() <= dijkstraPath(enemy, player2).size() || !multiplayer) ? player1 : player2;
        auto path = dijkstraPath(enemy, target);
        if (!path.empty()) enemy = path[0];

        if (enemy == player1) {
            health1--;
            if (health1 == 0) {
                clearScreen();
                cout << "\U0001F480 Game Over: Player 1 was caught!\nFinal Score: " << score << "\n";
                exit(0);
            }
            grid[player1.first][player1.second] = '.';
            player1 = {0, 0};
            grid[player1.first][player1.second] = '1';
        }

        if (multiplayer && enemy == player2) {
            health2--;
            if (health2 == 0) {
                clearScreen();
                cout << "\U0001F480 Game Over: Player 2 was caught!\nFinal Score: " << score << "\n";
                exit(0);
            }
            grid[player2.first][player2.second] = '.';
            player2 = {0, N - 1};
            grid[player2.first][player2.second] = '2';
        }

        grid[enemy.first][enemy.second] = 'E';
    }
}

void mainMenu() {
    clearScreen();
    cout << "===== Gridrun Escape Protocol =====\n";
    cout << "1. Single Player\n";
    cout << "2. Multiplayer\n";
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
            if (string("wasd").find(input) != string::npos)
                movePlayer(player1, health1, input, '1');
            else if (multiplayer && string("ijkl").find(input) != string::npos)
                movePlayer(player2, health2, input, '2');

            moveEnemies();
            printGrid();
            score++;
            this_thread::sleep_for(chrono::milliseconds(enemyMoveDelay));
        }
    }

    return 0;
}
