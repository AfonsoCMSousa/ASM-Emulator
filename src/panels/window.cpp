#include "window.h"

WINDOW *create_newPanel(int height, int width, int starty, int startx)
{
    WINDOW *local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 dá caracteres padrão para as linhas verticais and horizontais	*/
    wrefresh(local_win);  /* Mostra aquela caixa 	*/

    return local_win;
}

WINDOW *create_newPanel(int height, int width, int starty, int startx, const char *title)
{
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 dá caracteres padrão para as linhas verticais and horizontais	*/
    mvwprintw(local_win, 0, 2, "[ %s ]", title);
    wrefresh(local_win); /* Mostra aquela caixa 	*/
    return local_win;
}

void destroy_Panel(WINDOW *local_win)
{
    /* box (local_win, '', ''); : Isso não produzirá o resultado
     *  desejado de apagar a janela. Vai deixar seus quatro cantos,
     * e uma lembrança feia da janela.
     */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    /* Os parâmetros usados são
     * 1. win: a janela na qual operar
     * 2. ls: caractere a ser usado para o lado esquerdo da janela
     * 3. rs: caractere a ser usado para o lado direito da janela
     * 4. ts: caractere a ser usado na parte superior da janela
     * 5. bs: caractere a ser usado na parte inferior da janela
     * 6. tl: caractere a ser usado para o canto superior esquerdo da janela
     * 7. tr: caractere a ser usado no canto superior direito da janela
     * 8. bl: caractere a ser usado no canto inferior esquerdo da janela
     * 9. br: caractere a ser usado no canto inferior direito da janela
     */
    wrefresh(local_win);
    delwin(local_win);
}

void setTitle(WINDOW *local_win, const char *title)
{
    mvwprintw(local_win, 0, 2, "%s", title);
    wrefresh(local_win);
}

void print(WINDOW *local_win, int y, int x, const char *text)
{
    mvwprintw(local_win, y, x + 1, "%s", text);
    wrefresh(local_win);
}

void print(WINDOW *local_win, int y, int x, const char *text, int num)
{
    mvwprintw(local_win, y, x + 1, "%s%d", text, num);
    wrefresh(local_win);
}

void print8(WINDOW *local_win, int y, int x, const char *text, int num)
{
    mvwprintw(local_win, y, x + 1, "%s%08X", text, num);
    wrefresh(local_win);
}

void print16(WINDOW *local_win, int y, int x, const char *text, int num)
{
    mvwprintw(local_win, y, x + 1, "%s%016X", text, num);
    wrefresh(local_win);
}

void printff(WINDOW *local_win, int y, int x, const char *text, float num)
{
    mvwprintw(local_win, y, x + 1, "%s%032.5f", text, num);
    wrefresh(local_win);
}

void displayMemory(WINDOW *local_win, int mem_size, const CPU &core)
{
    uint16_t pc = core.getPC();

    for (int i = 0; i < mem_size; i += 4)
    {
        const char *dump_line = core.dump(i);

        // Check if the current memory address matches the PC
        if (i == pc || i + 1 == pc || i + 2 == pc || i + 3 == pc)
        {
            if (pc == core.getSC() - 4)
            {
                wattron(local_win, COLOR_PAIR(1));
                print(local_win, i / 4 + 1, 1, dump_line);
                wattroff(local_win, COLOR_PAIR(1));
            }
            else
            {
                wattron(local_win, COLOR_PAIR(5));
                print(local_win, i / 4 + 1, 1, dump_line);
                wattroff(local_win, COLOR_PAIR(5));
            }
        }
        else
        {
            print(local_win, i / 4 + 1, 1, dump_line); // Print normally
        }
    }
}

void displayStack(WINDOW *local_win, int stack_size, const CPU &core)
{
    if (stack_size > 256) // Check if the showed memory insnt bigger than the stack
    {
        stack_size = 256;
    }

    for (int i = 0; i < stack_size; i += 4)
    {
        const char *dump_line = core.getStack(i);
        if (i == core.getSP() || i + 1 == core.getSP() || i + 2 == core.getSP() || i + 3 == core.getSP())
        {
            if (i + 1 == core.getSP() || i + 2 == core.getSP() || i + 3 == core.getSP())
            {
                wattron(local_win, COLOR_PAIR(3));
                print(local_win, i / 4 + 1, 1, dump_line);
                wattroff(local_win, COLOR_PAIR(3));
            }
            else
            {
                wattron(local_win, COLOR_PAIR(5));
                print(local_win, i / 4 + 1, 1, dump_line);
                wattroff(local_win, COLOR_PAIR(5));
            }
        }
        else
        {
            print(local_win, i / 4 + 1, 1, dump_line); // Print normally
        }
    }
}

void printColor(WINDOW *local_win, int color, int y, int x, const char *text)
{
    wattron(local_win, COLOR_PAIR(color));
    mvwprintw(local_win, y, x + 1, "%s", text);
    wattroff(local_win, COLOR_PAIR(color));
    wrefresh(local_win);
}

void get_user_input(WINDOW *terminal, char *input, size_t size) {
    mvwprintw(terminal, 2, 1, "                                           "); // Clear the line for input
    mvwprintw(terminal, 2, 2, "> "); // Print cursor
    wrefresh(terminal); // Refresh to show cursor

    wgetnstr(terminal, input, size - 1); // Get user input
    wrefresh(terminal); // Refresh to show the entered text
}