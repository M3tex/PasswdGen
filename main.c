#include <stdio.h>
#include "passwd_utils.h"
#include "terminal_ui.h"


int main(void)
{
    clear_screen();
    enable_raw_mode();


    // printf("\033[30;107mHello\033[0m"); // Fond blanc
    while (1)
    {
        update_terminal();
    }
    return 0;
}