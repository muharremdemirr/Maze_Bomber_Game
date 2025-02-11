# Maze Bomber

Maze Bomber is a Bomberman-inspired strategic game featuring AI-driven opponents, dynamic maps, and multiplayer functionality. Players strategically place bombs to eliminate their opponents and be the last survivor. The game includes advanced AI bots, trap mechanisms, and power-ups.

## 🚀 Project Features

- **Single and Multiplayer Modes**  
  - Playable against AI-controlled bots.  
  - Supports local multiplayer.  
  - Online multiplayer mode (using ENet) planned for future versions.  

- **Advanced AI System**  
  - AI bots utilize escape, attack, and resource collection strategies.  
  - Implements A* pathfinding for optimal movement.  

- **Dynamic Map Structure**  
  - 15x15 grid-based game map.  
  - Destructible and indestructible walls for strategic gameplay.  
  - Power-ups and traps randomly distributed across the map.  

- **Real-Time Game Mechanics**  
  - Bomb placement and explosion effects.  
  - Trap mechanisms and power-up system.  
  - Real-time collision detection and movement handling.  

- **Visual and Audio Design**  
  - Custom-designed menu and in-game interface.  
  - FMOD integration for enhanced sound effects (future update).  

## 🛠 Technologies Used

- **Programming Language:** C++  
- **Graphics Library:** I-See-Bytes (ICBYTES)  
- **AI Algorithm:** A* Pathfinding  
- **Multithreading:** Windows Threads API  
- **Timing and Synchronization:** Chrono Library  
- **Online Multiplayer:** ENet (Planned Feature)  
- **Audio Engine:** FMOD (Planned Feature)  

## 🎮 How to Run?

### 1️⃣ Requirements

Ensure you have the following dependencies installed:

- **C++ compiler (GCC, MSVC, Clang)**  
- **ICBYTES Graphics Library**  
- **CMake (optional)**  

### 2️⃣ Build and Run

Clone the repository and build the project:

git clone https://github.com/username/MazeBomber.git
cd MazeBomber
mkdir build && cd build
cmake ..
make
./MazeBomber

## 🎯 Controls

| **Player 1**       | **Player 2**       |
|--------------------|--------------------|
| **W, A, S, D** - Move | **⬆️⬅️⬇️➡️** - Move |
| **E** - Place bomb  | **Numpad 0** - Place bomb |
| **Q** - Place trap  | **Numpad 3** - Place trap |

## 🔮 Future Developments

- 🌍 **Online Multiplayer:** Online matchmaking using ENet.  
- 🎙 **Voice Communication:** Real-time player interaction via voice chat.  
- 🎨 **Improved Graphics:** Better animations, explosion effects, and character movements.  
- 📱 **Cross-Platform Support:** Compatibility for PC, Mac, and Linux.  
- 🤖 **Smarter AI:** Machine learning-powered bots for better decision-making.  
