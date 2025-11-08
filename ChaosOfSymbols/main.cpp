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

    Game game;

    if (game.Initialize()) {
        game.Run();
    }
    else {
        cout << "\nFailed to initialize game!" << '\n';
        return -1;
    }

    cout << "\n\nThanks for playing!" << '\n';
    return 0;
}