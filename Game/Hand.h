#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
  public:
    Hand(Board *board) : board(board)
    {
        // Конструктор принимает указатель на объект Board для взаимодействия с игровым полем.
    }

    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;  // Установка типа ответа QUIT при закрытии окна.
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);  // Вычисление позиции клетки по клику мыши.
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;  // Установка типа ответа BACK для возврата хода.
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;  // Установка типа ответа REPLAY для перезапуска игры.
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;  // Установка типа ответа CELL для выбора клетки.
                    }
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();  // Изменение размера окна вызывает обновление размеров доски.
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;  // Выход из цикла при получении любого ответа, кроме OK.
            }
        }
        return {resp, xc, yc};  // Возврат типа ответа и выбранной позиции клетки.
    }

    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;  // Установка типа ответа QUIT при закрытии окна.
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size();  // Изменение размера окна вызывает обновление размеров доски.
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;  // Установка типа ответа REPLAY для перезапуска игры.
                }
                break;
                }
                if (resp != Response::OK)
                    break;  // Выход из цикла при получении любого ответа, кроме OK.
            }
        }
        return resp;  // Возврат типа ответа.
    }

  private:
    Board *board;  // Указатель на объект Board для взаимодействия с игровым полем.
};
