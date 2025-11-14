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

            // После завершения игры спросить, хочет ли игрок сыграть еще раз?
            // Или автоматически перезапускать
            // restartGame = AskToRestart(); 
            // Для простоты всегда перезапускаем
            restartGame = true;
        }
        else {
            cout << "\nFailed to initialize game!" << '\n';
            restartGame = false;
        }
    }

    cout << "\n\nThanks for playing!" << '\n';
    return 0;
}