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
    bool forceExit = false;

    while (restartGame && !forceExit) {
        Game game;

        if (game.Initialize()) {
            bool shouldExit = game.Run();
            if (shouldExit) {
                restartGame = false;
            }
            else {
                restartGame = true;
            }
        }
        else {
            restartGame = false;
        }
    }

    cout << "\n\nThanks for playing!" << '\n';

    return 0;
}