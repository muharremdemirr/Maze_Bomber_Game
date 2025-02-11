Maze Bomber

Maze Bomber is a strategic game inspired by Bomberman, featuring advanced AI bots, dynamic map generation, and multiplayer capabilities. The objective of the game is for players to strategically outmaneuver their opponents and be the last one standing.

Features

Single-Player and Multiplayer Modes: Play against AI bots or compete with friends.

Dynamic Map Generation: Each game starts with a different maze layout.

Advanced AI: AI bots have escape strategies, attack predictions, and resource collection abilities.

Power-Up System: Includes speed boost, bomb range extension, extra bombs, and shields.

Real-Time Explosion Mechanism: Players must strategically place bombs and calculate explosions.

FMOD Integration: FMOD sound engine is used for sound effects and music.

ENet Implementation (Coming Soon!): Online multiplayer support will be added.

Technologies

Language: C++

Graphics Library: I-See-Bytes (ICBYTES)

Sound Engine: FMOD

Pathfinding Algorithm: A* Algorithm

Network Connection (Future Update): ENet

Installation

Clone the repository:

git clone https://github.com/kullanici/MazeBomber.git
cd MazeBomber

Install dependencies:

Download the FMOD SDK and place it in the fmod/ directory.

Install the Boost library.

Compile and run the project:

mkdir build && cd build
cmake ..
make
./MazeBomber

Gameplay

Controls

Player 1

Player 2

Movement: W, A, S, D

Movement: Arrow Keys

Place Bomb: E

Place Bomb: Numpad 0

Set Trap: Q

Set Trap: Numpad 3

Future Plans

Online Multiplayer: Play matches with remote players using ENet.

Voice Chat: In-game voice communication for team strategies.

Advanced AI with Machine Learning: Bots will analyze and adapt their gameplay styles.

Improved Graphics and Animations: Smoother character movements and explosion effects.

Cross-Platform Support: Optimizing the game for Windows, Linux, and MacOS.

Contributing

To contribute, please open a pull request or share your ideas by creating an issue.

License

This project is licensed under the MIT License. For more details, see the LICENSE file.

Enjoy the fun and strategic gaming experience with Maze Bomber! 🎮🔥
