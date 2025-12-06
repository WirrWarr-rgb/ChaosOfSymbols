#include <iostream>
#include <windows.h>
#include "Game.h"

using namespace std;

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleTitleA("ChaosOfSymbols");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    bool restartGame = true;

    while (restartGame) {
        Game game;

        if (game.Initialize()) {
            game.Run();
            restartGame = true;
        }
        else {
            restartGame = false;
        }
    }

    cout << "\n\nThanks for playing!" << '\n';
    return 0;
}