#include <stdio.h>
#include "passwd_utils.h"
#include "terminal_ui.h"


int main(void)
{
    clear_screen();
    enable_raw_mode();
    init_terminal();

    while (1)
    {
        update_terminal();
    }
    return 0;
}