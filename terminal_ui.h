// Test

int getTerminalSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);
void clear_screen();
void init_terminal();
void enable_raw_mode();
void disable_raw_mode();
void editorProcessKeypress();
void editorRefreshScreen();
void abFree(struct abuf *ab);
void abAppend(struct abuf *ab, const char *s, int n);
void print_statusbar(struct abuf *ab);
void print_title(struct abuf *ab);
char editorReadKey();
void update_terminal();
void set_main_menu();
void set_passwd_menu();
void set_passphrase_menu();
void set_strenght_menu();

