#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#undef getch
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#define DEFAULT_WIDTH (80)
#define DEFAULT_HEIGHT (25)

void get_terminal_size(int *width, int *height) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *width = DEFAULT_WIDTH;
        *height = DEFAULT_HEIGHT;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        *width = w.ws_col;
        *height = w.ws_row;
    } else {
        *width = DEFAULT_WIDTH;
        *height = DEFAULT_HEIGHT;
    }
#endif
}

int get_keypress(void) {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

void clear_screen(void) {
    /* ANSI Escape Clear Screen */
    printf("\033[H\033[2J\033[3J");
    fflush(stdout);
}

bool is_double_clicked(void) {
#ifdef _WIN32
    DWORD process_list[2];
    DWORD count = GetConsoleProcessList(process_list, 2);
    return (count <= 1);
#else
    return false;
#endif
}

void ensure_terminal_size(int req_w, int req_h) {
    /* Ensure the console actually processes ANSI codes */
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut,
                       dwMode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
#endif

    int w, h;
    for(;;) {
        get_terminal_size(&w, &h);

        if (w >= req_w && h >= req_h) {
            break;
        }

        clear_screen();
        printf("--- TERMINAL TOO SMALL ---\n");
        printf("Current:  %dx%d\n", w, h);
        printf("Required: %dx%d\n", req_w, req_h);

        if (is_double_clicked()) {
            printf("\nHint: It looks like you double-clicked the app. Try "
                   "maximizing the window or using F11.\n");
        }

        printf("\nPlease resize and press Enter to retry, or Esc to quit.\n");

        int ch = get_keypress();
        if (ch == KEY_ESCAPE)
            exit(0);

        if (IS_ENTER(ch))
            continue;
    }

    clear_screen();
}
