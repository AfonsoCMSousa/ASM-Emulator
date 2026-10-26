#ifdef _WIN32
#include <ncurses/ncurses.h>
#include <ncursesw/curses.h>
#include <windows.h>
#else
#include <curses.h>
#include <ncurses.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <cstdint>
#include <map>

#include "panels/window.h"
#include "files/rw.h"

void sleep_ms(int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep takes microseconds
#endif
}

void printREGISTORS(const CPU &core, WINDOW *registers)
{
    print(registers, 1, 1, "Registers:");
    print8(registers, 2, 2, "A8: ", core.getREG8(0));
    print8(registers, 3, 2, "B8: ", core.getREG8(1));
    print8(registers, 4, 2, "C8: ", core.getREG8(2));
    print8(registers, 5, 2, "D8: ", core.getREG8(3));
    print8(registers, 6, 2, "PC: ", core.getPC());
    print16(registers, 2, 18, "A16: ", core.getREG16(0));
    print16(registers, 3, 18, "B16: ", core.getREG16(1));
    printff(registers, 4, 18, "AF:  ", core.getREGF(0));
    printff(registers, 5, 18, "BF:  ", core.getREGF(1));
    print8(registers, 6, 18, "SP:  ", core.getSP());
}

// Prints the source in the editor panel. Lines with assembler errors are shown
// in red with the error message appended.
void showSource(WINDOW *win, const std::vector<std::string> &lines, const std::vector<AsmError> &errors)
{
    std::map<int, std::string> errorAt;
    for (const auto &e : errors)
        if (!errorAt.count(e.line))
            errorAt[e.line] = e.message;

    const int maxLines = getmaxy(win) - 2;
    const size_t maxCols = getmaxx(win) > 4 ? static_cast<size_t>(getmaxx(win) - 4) : 0;

    for (int i = 0; i < static_cast<int>(lines.size()) && i < maxLines; ++i)
    {
        std::string text = lines[i];
        if (!text.empty() && text.back() == '\r')
            text.pop_back();

        std::ostringstream oss;
        oss << std::setw(3) << i + 1 << ": " << text;
        auto it = errorAt.find(i + 1);
        if (it != errorAt.end())
            oss << "   <-- " << it->second;

        std::string out = oss.str();
        if (out.size() > maxCols)
            out.resize(maxCols);

        if (it != errorAt.end())
            printColor(win, 1, i + 1, 1, out.c_str());
        else
            print(win, i + 1, 1, out.c_str());
    }
}

const char *BLANK = "                                                                                                    ";

// Shows the halt/fault message for a finished program.
void reportStop(WINDOW *info, const CPU &core)
{
    print(info, 1, 1, BLANK);
    if (core.getStatus() == CPU_FAULT)
    {
        std::string msg = "Fault: " + core.getFault() + ". Press [Q] to exit";
        printColor(info, 1, 1, 1, msg.c_str());
    }
    else
    {
        printColor(info, 3, 1, 1, "Info: Program has reached HALT! Press [Q] to exit");
    }
}

int main()
{
    CPU core;

    system("printf '\\e[8;46;203t'"); // Sends escape code to resize terminal

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    flushinp(); // Flush input buffer (must come after initscr)

    int width = 0;
    int height = 0;
    getmaxyx(stdscr, height, width);

    keypad(stdscr, TRUE); // Enable special keys like arrows
    curs_set(0);

    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);    // Red text on black background
        init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Green text on black background
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Yellow text on black background
        init_pair(4, COLOR_BLUE, COLOR_BLACK);   // Blue text on black background
        init_pair(5, COLOR_BLACK, COLOR_WHITE);  // Black text on white background
    }

    refresh();
    bool FileExists = true;
    intptr_t fd = -1;
    char input[256]; // Adjust the size as needed
    bool hasStoped = false;

    WINDOW *terminal = create_newPanel(5, (width / 7) * 4, 0, (width / 7) * 1, "Terminal panel");
    WINDOW *memory = create_newPanel(height - 3, (width / 7) * 1, 0, 0, "Memory panel");
    WINDOW *debug = create_newPanel(height - 8, (width / 7) * 4, 5, (width / 7) * 1, "Editor panel");
    WINDOW *registers = create_newPanel((height / 5) * 1, ((width / 7) * 2), 0, (width / 7) * 5, "Registors panel");
    WINDOW *program = create_newPanel(((height / 5) * 4) - 2, ((width / 7) * 2), (height / 5) * 1, (width / 7) * 5, "Stack panel");
    WINDOW *info = create_newPanel(3, width, height - 3, 0, "Info panel");

    echo();

    do
    {
        flushinp();
        refresh();
        print(terminal, 1, 1, "Enter the file name");
        get_user_input(terminal, input, sizeof(input) / sizeof(input[0]));

        if (input[0] == '\0')
        {
            printColor(terminal, 1, 3, 1, "Info: Please enter a valid file name                    ");
            FileExists = false;
            continue;
        }

        std::string new_filename = std::string(input) + ".s";
        snprintf(input, sizeof(input), "%s", new_filename.c_str());

        fd = open_file(input);
        if (fd <= -1)
        {
            printColor(terminal, 1, 3, 1, "Error: File does not exist                              ");
            FileExists = false;
        }
        else
        {
            FileExists = true;
        }
    } while (!FileExists);

    close_file(fd);

    delwin(terminal);
    terminal = create_newPanel(5, (width / 7) * 4, 0, (width / 7) * 1, "Terminal panel");
    noecho();

    try
    {
        std::vector<std::string> lines = read_lines(input);
        std::vector<AsmError> errors;

        if (core.assemble(lines, errors))
        {
            printColor(terminal, 2, 1, 1, "Info: File successfully loaded");
        }
        else
        {
            std::string msg = "Error: " + std::to_string(errors.size()) +
                              " assembly error(s), first on line " + std::to_string(errors[0].line) +
                              " (marked red in the editor)";
            printColor(terminal, 1, 1, 1, msg.c_str());
            hasStoped = true;
        }
        showSource(debug, lines, errors);
    }
    catch (const std::exception &e)
    {
        printColor(terminal, 1, 2, 1, "Error: File could not be read or had no information");
        hasStoped = true;
    }

    if (hasStoped)
        print(info, 1, 1, "Info: Fix the errors and restart. Press [Q] to quit");
    else
        print(info, 1, 1, "Info: Press [F] to go step-by-step or [R] to run the program. Press [Q] to quit");

    bool triedExit = false;
    bool showInfo = true;
    int speed = 300;

    while (!hasStoped)
    {
        printREGISTORS(core, registers);
        displayMemory(memory, (getmaxy(memory) * 4) - 8, core);
        displayStack(program, (getmaxy(program) * 4) - 8, core);
        int ch = getch();

        if ((ch == 'f' || ch == 'F'))
        {
            if (showInfo)
            {
                print(info, 1, 1, BLANK);
                printColor(info, 2, 1, 1, "Info: Program running on step-by-step mode");
                showInfo = false;
            }

            if (core.execute() != CPU_RUNNING)
            {
                printREGISTORS(core, registers);
                displayMemory(memory, (getmaxy(memory) * 4) - 8, core);
                displayStack(program, (getmaxy(program) * 4) - 8, core);
                reportStop(info, core);
                hasStoped = true;
            }
        }

        // Check for 'q' or 'Q' key press to quit
        if (ch == 'q' || ch == 'Q')
        {
            triedExit = true;
            break; // Exit the loop
        }

        if (ch == 'r' || ch == 'R')
        {
            print(info, 1, 1, BLANK);
            printColor(info, 4, 1, 1, "Info: Program running on automatic mode. Press [Q] to stop");
            nodelay(stdscr, TRUE); // let Q interrupt programs that loop forever
            while (core.execute() == CPU_RUNNING)
            {
                printREGISTORS(core, registers);
                displayMemory(memory, (getmaxy(memory) * 4) - 8, core);
                displayStack(program, (getmaxy(program) * 4) - 8, core);
                sleep_ms(speed);
                if (speed < 100)
                {
                    speed = 100;
                }
                else
                {
                    speed -= 5;
                }

                int key = getch();
                if (key == 'q' || key == 'Q')
                {
                    triedExit = true;
                    break;
                }
            }
            nodelay(stdscr, FALSE);

            if (!triedExit)
            {
                printREGISTORS(core, registers);
                displayMemory(memory, (getmaxy(memory) * 4) - 8, core);
                displayStack(program, (getmaxy(program) * 4) - 8, core);
                reportStop(info, core);
            }
            hasStoped = true;
        }
    }

    // Wait for Q unless the user already asked to quit
    if (!triedExit)
    {
        int ch;
        do
        {
            ch = getch();
        } while (ch != 'q' && ch != 'Q');
    }

    endwin(); // close the window
    return 0;
}
