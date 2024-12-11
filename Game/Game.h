#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        auto start = chrono::steady_clock::now();  // Запоминаем время начала игры для отслеживания длительности.

        if (is_replay)
        {
            logic = Logic(&board, &config);  // Пересоздаем логику для новой игры.
            config.reload();  // Перезагружаем конфигурацию.
            board.redraw();   // Перерисовываем игровую доску.
        }
        else
        {
            board.start_draw();  // Инициализация и отрисовка стартовой доски.
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns");
        while (++turn_num < Max_turns)
        {
            beat_series = 0;  // Сброс серии ударов.
            logic.find_turns(turn_num % 2);  // Поиск возможных ходов для текущего игрока.
            if (logic.turns.empty())
                break;  // Выход из цикла, если больше нет доступных ходов.

            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ход игрока
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT)
                {
                    is_quit = true;  // Завершение игры, если игрок выбрал выход.
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;  // Перезапуск игры, если игрок выбрал повтор.
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Обработка возврата хода
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2);  // Ход бота.
        }

        auto end = chrono::steady_clock::now();  // Запоминаем время окончания игры.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay)
            return play();  // Повторный запуск игры, если выбрано повторение.

        if (is_quit)
            return 0;  // Завершение игры без результата.

        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0;  // Ничья, если достигнуто максимальное количество ходов.
        }
        else if (turn_num % 2)
        {
            res = 1;  // Победа одного из игроков в зависимости от очереди хода.
        }
        board.show_final(res);  // Показ результата игры.
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();  // Повторный запуск игры при выборе повторения.
        }
        return res;  // Возврат результата игры.
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();  // Запоминаем время начала хода бота.

        auto delay_ms = config("Bot", "BotDelayMS");
        // Новый поток для обеспечения задержки перед ходом.
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);  // Поиск лучших ходов для бота.
        th.join();
        bool is_first = true;

        // Выполнение найденных ходов.
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);  // Задержка между ходами, если это не первый ход.
            }
            is_first = false;
            beat_series += (turn.xb != -1);  // Увеличение серии ударов, если ход с побитием.
            board.move_piece(turn, beat_series);  // Перемещение фигуры на доске.
        }

        auto end = chrono::steady_clock::now();  // Запоминаем время окончания хода бота.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();  // Запись времени хода бота в лог-файл.
    }
    
    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);  // Добавление всех возможных начальных позиций в список для выделения.
        }
        board.highlight_cells(cells);  // Выделение возможных начальных позиций на доске.
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;

        // Попытка сделать первый ход
        while (true)
        {
            auto resp = hand.get_cell();  // Ожидание ввода от пользователя для выбора клетки.
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);  // Возврат, если пользователь решил выйти или перезапустить игру.
            
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;  // Проверка корректности выбранной начальной позиции.
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn;  // Проверка правильности хода и установка текущего хода.
                    break;
                }
            }
            if (pos.x != -1)
                break;  // Выход из цикла, если ход корректен.
            
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();  // Очистка активных и выделенных клеток, если выбор некорректен.
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);  // Установка активной клетки после корректного выбора.
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);  // Добавление возможных конечных позиций для выбранной начальной позиции.
                }
            }
            board.highlight_cells(cells2);  // Выделение возможных конечных позиций на доске.
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);  // Перемещение фигуры.

        if (pos.xb == -1)
            return Response::OK;  // Возврат OK, если ударов больше нет.

        // Продолжение серии ударов, если возможно
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);  // Поиск возможных продолжений серии ударов.
            if (!logic.have_beats)
                break;  // Завершение серии, если ударов больше нет.

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);  // Добавление конечных позиций для продолжения серии.
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);

            // Попытка сделать следующий ход в серии
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);  // Возврат, если пользователь решил выйти или перезапустить игру.
                
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;  // Установка текущего хода для продолжения серии.
                        break;
                    }
                }
                if (!is_correct)
                    continue;  // Продолжение цикла, если ход некорректен.

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;  // Увеличение серии ударов.
                board.move_piece(pos, beat_series);
                break;  // Завершение цикла, если ход корректен.
            }
        }

        return Response::OK;  // Возврат OK после завершения всех ходов.
}

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
