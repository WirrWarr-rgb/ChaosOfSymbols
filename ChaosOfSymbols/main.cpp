#include <iostream>
#include <windows.h>
#include "Game.h"

using namespace std;

/// <summary>
/// Установка кодировки, заголовока и информации о курсоре
/// </summary>
void SetupConsole() {
    SetConsoleOutputCP(65001);

    SetConsoleTitleA("ChaosOfSymbols");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

/// <summary>
/// ОСНОВА
/// </summary>
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