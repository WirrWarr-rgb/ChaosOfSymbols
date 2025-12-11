# Chaos Of Symbols

[![C++](https://img.shields.io/badge/C++-17+-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/Status-In%20Development-orange.svg)](https://github.com/WirrWarr-rgb/ChaosOfSymbols)

A C++ console game with flexible gameplay options, a procedurally generated world, and a world that changes according to cellular automata rules.

<img width="1814" height="579" alt="COS9" src="https://github.com/user-attachments/assets/1b24fc2f-070a-4242-8d6e-eedfa6330343" />

## Features

- ASCII-art Console Interface - Full keyboard control with intuitive menu navigation
- Symbol-based World Representation - Every game object (terrain, entities, items) is represented by a single character symbol
- Procedural world generation — worlds using noise
- 1 time tick = 1 player action
- Cellular automata — the world changes every time tick according to cellular automata rules
- Each tile has its own cellular automaton rules
- Food, experience, levels — you can eat food to increase experience and levels
- Colorful console — improved visual perception using rlutil
- Flexible customization of many world parameters

## Gameplay & Controls

**The game can be divided into two parts:**
1. Editor
2. Gameplay

**Editor:** You can edit templates and world data, load them into saves, and play.

**Gameplay:** You can move around the world itself, eat food, and gain experience and level.

### Game Overview

### Controls
**Menu**
- [W] [S] or [↑] [↓] - Move between menu options
- [A] [D] or [←] [→] - Decrease and increase selected values
- [Space] or [Enter] - Confirm
- [Esc] - Back
- [Tab] or [Shift]  + [Tab] - Move between World Editor tabs
- [Ctrl] + [C] and [Ctrl] + [V] - use to add cellular automaton rules

**Game**
- [W] [A] [S] [D] - Move
- [Esc] - Back

## Screenshots & Examples
**World Editor**

<img width="717" height="497" alt="COS1" src="https://github.com/user-attachments/assets/c1620ca4-6885-45fe-b139-85dfea6d1aac" />

**Game Saves**

<img width="716" height="496" alt="COS5" src="https://github.com/user-attachments/assets/ee611f7f-80e6-46aa-812d-dee3a521b39a" />

**World Templates**

<img width="717" height="494" alt="COS4" src="https://github.com/user-attachments/assets/6d1985f1-c136-4a2a-9563-e62bfa0c4073" />

**Gameplay**

<img width="628" height="578" alt="COS7" src="https://github.com/user-attachments/assets/2a6d8060-9fcb-408d-813b-2f0fd2b9f57b" />
<img width="628" height="578" alt="COS8" src="https://github.com/user-attachments/assets/bcb20d9e-8cac-4bd3-8a6a-2fadf7b6d33e" />

<img width="630" height="877" alt="COS10" src="https://github.com/user-attachments/assets/f5fe9d7c-0492-45e3-b3d0-6b4d57d048e2" />
 
## Development Setup

### Prerequisites
Visual Studio 2022 (recommended) or 2019
Git for version control
C++ Compiler supporting at least C++17

### Installation with Visual Studio

1. **Clone the repository**
git clone https://github.com/WirrWarr-rgb/ChaosOfSymbols.git
cd WirrWarr-rgb/ChaosOfSymbols

2. **Open in Visual Studio**
   - Open `ChaosOfSymbols.sln` solution file
   - Wait for Visual Studio to load the project

3. **Build and Run**
   - Press `F5` to build and run in Debug mode
   - Or use `Ctrl+F5` to run without debugging

4. **Run the game**
   - Set the game project as Startup Project
   - Run with F5 (Debug) or Ctrl+F5 (Start Without Debugging)

## Dependencies

**[rlutil](https://github.com/tapio/rlutil)** - simple utility collection to aid the creation of cross-platform console-mode roguelike games with C++ and C.

**[FastNoiseLite](https://github.com/Auburn/FastNoiseLite)** - an extremely portable open source noise generation library with a large selection of noise algorithms. This library focuses on high performance while avoiding platform/language specific features, allowing for easy ports to as many possible languages.

## Academic Context

This project is being developed as part of university course.

## Documentation

- [Contributing Guidelines](CONTRIBUTING.md) - How to contribute
- [Code of Conduct](CODE_OF_CONDUCT.md) - Community standards
- [Security Policy](SECURITY.md) - Security reporting
- [License](LICENSE) - MIT License details

## Development Status

**Current Status:** In development. But whether further development will actually take place is still unknown.

## License
This project is licensed under the MIT License.

## Credit
wirrwarr

<img width="1817" height="578" alt="COS11" src="https://github.com/user-attachments/assets/b0b7382c-eed0-4413-8e01-854f63f417c4" />
