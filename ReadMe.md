
# Gridrun Escape Protocol

A terminal-based C++ survival game where you navigate a grid, evade enemies, and reach safety!

## 🚀 Features

- **20x20 Grid**: Player starts at (0, 0)
- **Enemies**: Use Dijkstra's Algorithm to chase you
- **Power-ups**: Heal at `+`
- **Safe Point**: Reach `X` to level up
- **Obstacles**: Random `#` blocks
- **Pause/Resume**: `p` key
- **Exit**: `q` key

## 🕹 Controls

- `w` - Move Up  
- `s` - Move Down  
- `a` - Move Left  
- `d` - Move Right  
- `p` - Pause/Resume  
- `q` - Quit

## 🧠 AI Logic

Enemies use **Dijkstra’s Algorithm** to find the shortest path to the player. Every enemy moves closer after each of the player’s moves.

## 🏆 Scoring

- +10 × Level upon reaching safe point
- +1 per valid move
- Lose 1 health if caught by enemy
- Game over at 0 health

## 🔁 Level Progression

Each level:
- Adds more enemies
- Speeds up enemy movement
- Introduces more obstacles

## 💻 Terminal Requirement

Uses `termios` for non-blocking input (Unix only).

## 🧩 Compile & Run

```bash
g++ -std=c++17 -o gridrun gridrun.cpp
./gridrun
```

## 📌 TODO

- Add multiplayer
- Power-up variety
- Smarter enemy AI
- Save/load game state

---


<img width="1203" alt="Game begins" src="https://github.com/user-attachments/assets/d1dd1e91-289a-4e6e-8db6-228536ee2238" />
<img width="1213" alt="During Game" src="https://github.com/user-attachments/assets/5f891f87-c6c0-44e1-9ece-ea941a78736c" />
<img width="1214" alt="Game Over!!" src="https://github.com/user-attachments/assets/f08f5b76-0282-47a2-930d-41635792e82f" />


Have fun escaping... while you still can. 🏃💨
