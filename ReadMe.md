
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

Have fun escaping... while you still can. 🏃💨
