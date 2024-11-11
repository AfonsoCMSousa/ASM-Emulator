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

void printREGISTORS(CPU core, WINDOW *registers)
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
void init_colors()
{
    if (has_colors() == FALSE)
    {
        endwin();
        std::cerr << "Your terminal does not support color." << std::endl;
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Define color pair 1 as green text on a black background
}

void write(WINDOW *win, std::string program, CPU &core)
{
    static int line = 1;
    core.load(program);
    std::ostringstream oss;
    oss << std::setw(3) << line << ": " << program;
    std::string line_number = oss.str();
    const char *text = line_number.c_str();
    if (line < getmaxy(win) - 1)
    {
        print(win, line, 1, text);
    }
    line++;
}

int main()
{
    CPU core;

    system("printf '\\e[8;46;203t'");  // Sends escape code to resize terminal
    flushinp();                        // Flush input buffer

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();

    int width = SCREEN_HEIGHT;
    int height = SCREEN_WIDTH;

    int start_y = 1;
    int start_x = 1;

    getmaxyx(stdscr, height, width);

    WINDOW *win = newwin(height, width, start_y, start_x);
    WINDOW *memory = newwin(138, 60, 0, 0);
    WINDOW *registers = newwin(5, 60, 0, 60);
    WINDOW *program = newwin(106, 60, 5, 60);
    WINDOW *terminal = newwin(5, 180, 0, 120);
    WINDOW *info = newwin(3, 240, 5, 0);
    WINDOW *debug = newwin(138, 180, 0, 120);
    
    wrefresh(win);

    cbreak();
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
    intptr_t fd;
    char input[256]; // Adjust the size as needed
    bool hasStoped = false;

    terminal = create_newPanel(5, (width / 7) * 4, 0, (width / 7) * 1, "Terminal panel");
    memory = create_newPanel(height - 3, (width / 7) * 1, 0, 0, "Memory panel");
    debug = create_newPanel(height - 8, (width / 7) * 4, 5, (width / 7) * 1, "Editor panel");
    registers = create_newPanel((height / 5) * 1, ((width / 7) * 2), 0, (width / 7) * 5, "Registors panel");
    program = create_newPanel(((height / 5) * 4) - 2, ((width / 7) * 2), (height / 5) * 1, (width / 7) * 5, "Stack panel");
    info = create_newPanel(3, width, height - 3, 0, "Info panel");

    echo();

    do
    {
        flushinp();
        refresh();
        print(terminal, 1, 1, "Enter the file name");
        get_user_input(terminal, input, sizeof(input) / sizeof(input[0]));

        std::string new_filename = std::string(input) + ".s";
        strcpy(input, new_filename.c_str());

        if (input[0] == '\0')
        {
            printColor(terminal, 3, 3, 1, "Info: Please enter a valid file name");
            FileExists = false;
        }
        else
        {
            fd = open_file(input);
            if (fd <= -1)
            {
                printColor(terminal, 1, 2, 1, "Error: File does not exist                              ");
                FileExists = false;
            }
            else
            {
                FileExists = true;
            }
        }
    } while (!FileExists);

    if (fd != -1)
    {
        close_file(fd);
    }

    terminal = create_newPanel(5, (width / 7) * 4, 0, (width / 7) * 1, "Terminal panel");
    noecho();

    printColor(terminal, 2, 1, 1, "Info: File successfully loaded");

    try
    {
        std::vector<std::string> lines = read_lines(input);

        for (auto line : lines)
        {
            write(debug, line, core);
        }
    }
    catch (const std::runtime_error &e)
    {
        printColor(terminal, 3, 2, 1, "Error: File could not be read or had no information");
        hasStoped = true;
    }

    print(info, 1, 1, "Info: Press [F] to go step-by-step or [R] to run the program. Press [Q] to quit");

    bool triedExit = false;
    bool showInfo = true;
    int temp = 0;
    int speed = 300;

    while (!hasStoped)
    {
        printREGISTORS(core, registers);
        displayMemory(memory, (getmaxy(memory) * 4) - 8, core);
        displayStack(program, (getmaxy(program) * 4) - 8, core);
        int ch = getch(); // Get user input, non-blocking due to nodelay

        if ((ch == 'f' || ch == 'F'))
        {
            if (showInfo)
            {
                print(info, 1, 1, "                                                                                                    ");
                printColor(info, 2, 1, 1, "Info: Program running on step-by-step mode");
                showInfo = false;
            }

            if (core.execute() == 1 || core.getSC() == 0)
            {
                print(info, 1, 1, "                                                                                                    ");
                printColor(info, 3, 1, 1, "Info: Program has reached HALT! Press [Q] to exit");
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
            print(info, 1, 1, "                                                                                                    ");
            printColor(info, 4, 1, 1, "Info: Program running on automatic mode");
            while (core.execute() != 1)
            {
                printREGISTORS(core, registers);
                displayMemory(memory, 138, core);
                displayStack(program, 106, core);
                sleep_ms(speed);
                if (speed < 100)
                {
                    speed = 100;
                } else {
                    speed -= 5;
                }
            }
            print(info, 1, 1, "                                                                                                    ");
            printColor(info, 3, 1, 1, "Info: Program has reached HALT! Press [Q] to exit");
            hasStoped = true;
            break;
        }
    }

    while (getch() != 'q' || triedExit)
    {
    }

    endwin(); // close the window
    return 0;
}