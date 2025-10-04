#include "Game.h"
#include <iostream>
#include <windows.h>

void SetupConsole() {
    SetConsoleOutputCP(65001);

    SetConsoleTitleA("ASCII Adventure Game");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

int main() {
    SetupConsole();

    std::cout << "Starting ASCII Adventure Game!" << std::endl;
    std::cout << "Controls: WASD/Arrows - move, Q - quit, R - regenerate, K - save, L - load" << std::endl;
    std::cout << "Initializing..." << std::endl;

    Game game;
    if (game.Initialize()) {
        game.Run();
    }
    else {
        std::cout << "Failed to initialize game!" << std::endl;
        return -1;
    }

    std::cout << "Thanks for playing!" << std::endl;
    return 0;
}