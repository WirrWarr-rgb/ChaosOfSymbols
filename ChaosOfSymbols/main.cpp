#include "Game.h"
#include <iostream>
#include <windows.h>

using namespace std;

/// <summary>
/// Устанавливает кодировку, заголовок и информацию о курсоре
/// </summary>
void SetupConsole() {
    SetConsoleOutputCP(65001);

    SetConsoleTitleA("ChaosOfSymbols");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

int main() {
    SetupConsole();

    Game game;
    
    if (game.Initialize()) {
        game.Run();
    }
    else {
        cout << "Failed to initialize game!" << '\n';
        return -1;
    }

    cout << "Thanks for playing!" << '\n';
    return 0;
}