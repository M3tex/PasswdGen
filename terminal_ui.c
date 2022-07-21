/*
Ce fichier contiendra toutes les fonctions nécessaire 
à l'affichage de l'interface dans le terminal
*/
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#include "passwd_utils.h"



/*** Définitions & Structures ***/

#define CTRL_KEY(k) ((k) & 0x1f)

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif


struct editorConfig 
{
    bool main_menu;
    bool passwd_menu;
    bool passphrase_menu;
    bool strenght_menu;

    int screenrows;
    int screencols;
    struct termios orig_termios;
};
struct editorConfig E;


struct abuf
{
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}



/*** Variables globales ***/

char *VERSION = "Password Generator v0.1";
char *AUTHOR = "Built by M3tex ";
char *HELP = " ctrl + h for help";

char *TITLE_1 = "#######\\                                        ######\\                      \r\n";
char *TITLE_2 = "##  __##\\                                      ##  __##\\                    \r\n";
char *TITLE_3 = "## |  ## |######\\   #######\\  #######\\         ## /  \\__| ######\\  #######\\  \r\n";
char *TITLE_4 = "#######  |\\____##\\ ##  _____|##  _____|######\\ ## |####\\ ##  __##\\ ##  __##\\ \r\n";
char *TITLE_5 = "##  ____/ ####### |\\######\\  \\######\\  \\______|## |\\_## |######## |## |  ## |\r\n";
char *TITLE_6 = "## |     ##  __## | \\____##\\  \\____##\\         ## |  ## |##   ____|## |  ## |\r\n";
char *TITLE_7 = "## |     \\####### |#######  |#######  |        \\######  |\\#######\\ ## |  ## |\r\n";
char *TITLE_8 = "\\__|      \\_______|\\_______/ \\_______/          \\______/  \\_______|\\__|  \\__|\r\n";

char *WELCOME1 = " Welcome to PasswdGen ! This tool is built to generate strong passwords, using a CSPRNG.\r\n";
char *WELCOME2 = " It has several modes: password generator, passphrase generator and password strength tester !\r\n\r\n";
char *WELCOME3 = " To get started select a mode: \r\n";

char *MODE1 = "~Password Generator               Passphrase Generator                Strenght Tester\r\n";
char *MODE2 = "~    (ctrl + g)                        (ctrl + p)                        (ctrl + t)  \r\n";




/*** Prototype des fonctions ***/

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



/*** Fonctions ***/

int getTerminalSize(int *rows, int *cols) 
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}


int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) 
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }

    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}


/**
 * @brief Permet d'effacer le terminal
 * 
 */
void clear_screen()
{
    system(CLEAR);
}


void init_terminal()
{
    // Appelée au démarrage du programme
    if (getTerminalSize(&E.screenrows, &E.screencols) == -1) die("Unable to compute terminal size");
    set_main_menu();
}


void update_terminal()
{
    // Appelée à chaque fois que l'interface est mise à jour
    if (getTerminalSize(&E.screenrows, &E.screencols) == -1) die("Unable to compute terminal size");

    // Efface le curseur
    printf("\x1b[?25l");
    editorRefreshScreen();
    clear_input();
    editorProcessKeypress();
}


void set_main_menu()
{
    E.main_menu = true;
    E.passwd_menu = false;
    E.passphrase_menu = false;
    E.strenght_menu = false;
}


void set_passwd_menu()
{
    E.main_menu = false;
    E.passwd_menu = true;
    E.passphrase_menu = false;
    E.strenght_menu = false;
}


void set_passphrase_menu()
{
    E.main_menu = false;
    E.passwd_menu = false;
    E.passphrase_menu = true;
    E.strenght_menu = false;
}


void set_strenght_menu()
{
    E.main_menu = false;
    E.passwd_menu = false;
    E.passphrase_menu = false;
    E.strenght_menu = true;
}


/**
 * @brief Permet de modifier les propriétés du terminal telles qu'on veut.
 * On passe en "raw mode" -> les combinaisons du types CTRL+C ou CTRL+Z sont
 * désactivées.
 */
void enable_raw_mode() 
{
    // On récupère les attributs du terminal dans une variable globale
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("Unable to get terminal attributes");
    atexit(disable_raw_mode);     // Quand main return ou que exit() est call.

    struct termios raw = E.orig_termios;

    /* On désactive l'écho et le mode canonique en modifiant les bitflags
    Le mode canonique lit les entrées lignes par lignes (i.e il faut appuyer sur entrée pour
    que le programme lise). On lit maintenant bit par bit.
    De même on désactive les combinaisons de touches (CTRL+C, CTRL+Z, CTRL+S, CTRL+Q)
    */

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("Unable to modify terminal attributes");
}


/**
 * @brief Fonction qui rétablit les propriétés du terminal telles
 * qu'elles étaient avant le lancement du programme.
 */
void disable_raw_mode() 
{
    /* On remet les propriétés qui étaient définies avant le lancement du programme
    TCSAFLUSH permet de supprimer les inputs non lus par le programme */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("Unable to reset terminal attributes");
}


/**
 * @brief Gère une entrée clavier et permet de définir les combinaisons du type CTRL+Q -> quitter le programme
 * 
 */
void editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        clear_screen();
        // On réaffiche le curseur
        printf("\x1b[?25h");
        exit(0);
        break;
    case CTRL_KEY('c'):
        clear_screen();
        clear_input();
        break;
    }
}


/**
 * @brief Permet de mettre à jour l'affichage
 * 
 */
void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;

    // Permet de cacher le curseur
    abAppend(&ab, "\x1b[?25l", 6);

    /* On efface l'écran avec la commande J (Erase in Display). Le 2 spécifie d'effacer tout l'écran.
    On pourrait également effacer du haut jusqu'au curseur avec 1, ou du curseur jusqu'au bas avec 0.
    Attention car ici le curseur est replacé à la fin. */
    abAppend(&ab, "\x1b[2J", 4);

    //clear_screen();
    /* On repositionne le curseur au bon endroit avec la commande H (Cursor Position).
    La commande H prend 2 arguments: le numéro de ligne et le numéro de colonne.
    Par défaut, le numéro de ligne est 1 et le numéro de colonne est 1 (on laisse donc par défaut,
    mais on pourrait mettre \x1b[1;1H si on le souhaitait)
    */
    abAppend(&ab, "\x1b[H", 3);

    // On affiche le titre
    // print_title(&ab);


    // On affiche la status bar
    // print_statusbar(&ab);

    // On repositionne en haut à gauche
    // TODO: le repositionner sous le titre
    // abAppend(&ab, "\x1b[H", 3);

    // On affiche la fenêtre actuelle
    if (E.main_menu)
    {
        // TODO: Fonction pour afficher le menu principal
        print_main_menu(&ab);
    }
    else if (E.passwd_menu)
    {
        // TODO: Fonction pour afficher le menu de génération de mot de passe
    }
    else if (E.passphrase_menu)
    {
        // TODO: Fonction pour afficher le menu de génération de phrase de passe
    }
    else if (E.strenght_menu)
    {
        // TODO: Fonction pour afficher le menu de test de force du mot de passe
    }

    // Permet de réafficher le curseur
    // abAppend(&ab, "\x1b[?25h", 6);

    // On écrit le contenu de l'abuf dans le terminal
    write(STDOUT_FILENO, ab.b, ab.len);

    // On libère toute la mémoire allouée
    abFree(&ab);
}


void print_title(struct abuf *ab)
{
    char *title[8] = {TITLE_1, TITLE_2, TITLE_3, TITLE_4, TITLE_5, TITLE_6, TITLE_7, TITLE_8};
    int nb_spaces = (E.screencols - strlen(title[0]) + 2) / 2;

    char *spaces = malloc(sizeof(char) * nb_spaces + 1);
    spaces[nb_spaces] = '\0';
    memset(spaces, ' ', nb_spaces);
    
    abAppend(ab, "\r\n", 2);
    for (int i = 0; i < 8; i++)
    {
        abAppend(ab, spaces, nb_spaces);
        abAppend(ab, title[i], strlen(title[i]));
    }
    abAppend(ab, "\r\n", 2);
    free(spaces);
}


void print_statusbar(struct abuf *ab)
{
    // On positionne le curseur en bas à gauche
    char *nb[1] = {int2str(E.screenrows)};
    char *pos = format_str("\x1b[~;H", nb, 1, '~');
    abAppend(ab, pos, strlen(pos));
    free(nb[0]);
    free(pos);

    // On centre le message
    int nb_spaces = (E.screencols - (strlen(HELP) + strlen(VERSION) + strlen(AUTHOR))) / 2;

    char *spaces = malloc(sizeof(char) * nb_spaces + 1);
    spaces[nb_spaces] = '\0';
    memset(spaces, ' ', nb_spaces);

    char *args[5] = {HELP, spaces, VERSION, spaces, AUTHOR};
    char *status = format_str("\033[30;107m~~~~~\033[0m", args, 5, '~');
    abAppend(ab, status, strlen(status));
    free(spaces);
    free(status);
}


void print_main_menu(struct abuf *ab)
{
    // On affiche le titre
    print_title(ab);

    // On affiche le message de bienvenue
    char *menu[3] = {WELCOME1, WELCOME2, WELCOME3};
    abAppend(ab, "\r\n\r\n", 4);
    
    for (int i = 0; i < 3; i++)
    {
        abAppend(ab, menu[i], strlen(menu[i]));
    }

    // Calcule les espaces entre les modes
    char *mode[2] = {MODE1, MODE2};
    int center_modes = (E.screencols - strlen(mode[0]) - 3) / 2;

    char *spaces1 = malloc(sizeof(char) * center_modes + 1);
    spaces1[center_modes] = '\0';
    memset(spaces1, ' ', center_modes);

    char *line1_args[1] = {spaces1};
    char *line1 = format_str(MODE1, line1_args, 1, '~');

    char *line2 = format_str(MODE2, line1_args, 1, '~');

    // On peut afficher 
    abAppend(ab, line1, strlen(line1));
    abAppend(ab, line2, strlen(line2));

    // On libère la mémoire
    free(spaces1);
    free(line1);
    free(line2);
    

    // On affiche la status bar
    print_statusbar(ab);

    // On repositionne le curseur en haut à gauche
    abAppend(ab, "\x1b[H", 3);
}


void abAppend(struct abuf *ab, const char *s, int n)
{
    char *new = realloc(ab->b, ab->len + n);

    if (new == NULL) return;

    memcpy(&new[ab->len], s, n);
    ab->b = new;
    ab->len += n;
}


void abFree(struct abuf *ab)
{
    free(ab->b);
}


/**
 * @brief Lit un caractère depuis le terminal et le retourne si il est valide.
 * 
 * @return Le caractère lu
 */
char editorReadKey() 
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) 
    {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}