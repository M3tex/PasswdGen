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
#include <time.h>
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
    bool lock;
    char *menu;

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


struct passwd_menu_data
{
    int nb_passwd;
    int nb_chars;
    char *special_chars;
    char **passwords;

    int selected_param;
    int special_idx;
    bool generate;
    double exec_time;
};
struct passwd_menu_data PasswordParams;


struct passphrase_menu_data
{
    int nb_passphrase;
    int nb_words;
    bool spell_word;
    char *word2spell; // Optionnel
};
struct passphrase_menu_data PassphraseParams;



/*** Variables globales ***/

// Status Bar
char *SB_MIDDLE = "Password Generator v0.1";
char *SB_RIGHT = "Built by M3tex ";
char *SB_LEFT = " ctrl + h for help";

// Titre
char *TITLE_1 = "#######\\                                        ######\\                      \r\n";
char *TITLE_2 = "##  __##\\                                      ##  __##\\                    \r\n";
char *TITLE_3 = "## |  ## |######\\   #######\\  #######\\         ## /  \\__| ######\\  #######\\  \r\n";
char *TITLE_4 = "#######  |\\____##\\ ##  _____|##  _____|######\\ ## |####\\ ##  __##\\ ##  __##\\ \r\n";
char *TITLE_5 = "##  ____/ ####### |\\######\\  \\######\\  \\______|## |\\_## |######## |## |  ## |\r\n";
char *TITLE_6 = "## |     ##  __## | \\____##\\  \\____##\\         ## |  ## |##   ____|## |  ## |\r\n";
char *TITLE_7 = "## |     \\####### |#######  |#######  |        \\######  |\\#######\\ ## |  ## |\r\n";
char *TITLE_8 = "\\__|      \\_______|\\_______/ \\_______/          \\______/  \\_______|\\__|  \\__|\r\n";

// Main Menu
char *WELCOME1 = " Welcome to PasswdGen ! This tool is built to generate strong passwords, using a CSPRNG.\r\n";
char *WELCOME2 = " It has several modes: password generator, passphrase generator and password strength tester !\r\n\r\n";
char *WELCOME3 = " To get started select a mode: \r\n\r\n";

char *MODE1 = "~Password Generator               Passphrase Generator                Strenght Tester\r\n";
char *MODE2 = "~    (ctrl + g)                        (ctrl + p)                        (ctrl + t)  \r\n";

// Password Menu
char *PASSWD_WELCOME = "~Welcome to the password generator.\r\n\r\n";
char *PASSWD_EXPLAIN1 = "~Use up and down arrows to select a parameter (marked by '\033[96m>\033[0m').\r\n";
char *PASSWD_EXPLAIN2 = "~Use left and right arrows to modify the selected parameter.\r\n";
char *PASSWD_EXPLAIN3 = "~Once done, simply press ctrl + g !\r\n\r\n";

char *PASSWD_PARAM1 = "~~Number of passwords: ~\r\n";
char *PASSWD_PARAM2 = "~~Number of characters: ~\r\n";
char *PASSWD_PARAM3 = "~~Special characters: ~\r\n";


// Passphrase Menu


// Strenght Tester Menu



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
void print_main_menu(struct abuf *ab);
void print_passwd_menu(struct abuf *ab);
char editorReadKey();
void update_terminal();
void set_main_menu();
void set_passwd_menu();
void set_passphrase_menu();
void set_strenght_menu();
void print_passwd_result(struct abuf *ab);
void free_passwd_buffer();


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

    // Permet de cacher le curseur
    printf("\x1b[?25l");
    set_main_menu();
}


void update_terminal()
{
    // Appelée à chaque fois que l'interface est mise à jour
    if (getTerminalSize(&E.screenrows, &E.screencols) == -1) die("Unable to compute terminal size");

    editorRefreshScreen();
    clear_input();
    editorProcessKeypress();
}


void set_main_menu()
{
    // On réinitialise les variables
    E.menu = "main";
    E.lock = false;
    PasswordParams.generate = false;

    // La status bar est réinitialisée
    SB_MIDDLE = "Password Generator v0.1";
    SB_RIGHT = "Built by M3tex ";
    SB_LEFT = " ctrl + h for help";
}


void set_passwd_menu()
{
    // On réinitialise les variables
    E.menu = "passwd";
    E.lock = false;

    // On initialise avec les paramètres par défaut
    PasswordParams.nb_passwd = 5;
    PasswordParams.nb_chars = 12;
    PasswordParams.selected_param = 0;
    PasswordParams.special_idx = 0;
}


void set_passphrase_menu()
{

    E.menu = "passphrase";
    E.lock = false;
}


void set_strenght_menu()
{

    E.menu = "strenght";
    E.lock = false;
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
        free_passwd_buffer();
        clear_screen();
        // On réaffiche le curseur
        printf("\x1b[?25h");
        exit(0);
        break;
    case CTRL_KEY('c'):
        clear_screen();
        clear_input();
        break;
    case CTRL_KEY('g'):
        if (E.menu == "passwd")
        {
            E.menu = "passwd_result";
            editorRefreshScreen();
            break;
        }
        else if (E.menu == "passwd_result")
        {
            E.lock = false;
            editorRefreshScreen();
            break;
        }
        set_passwd_menu();
        break;
    case CTRL_KEY('m'):
        set_main_menu();
        break;
    case 'Z':
        if (E.menu == "passwd" && PasswordParams.selected_param > 0 && !(E.menu == "passwd_result"))
        {
            PasswordParams.selected_param--;
            editorRefreshScreen();
        }
        break;
    case 'S':
        if (E.menu == "passwd" && PasswordParams.selected_param < 2 && !(E.menu == "passwd_result"))
        {
            PasswordParams.selected_param++;
            editorRefreshScreen();
        }
        break;
    case 'Q':
        if (E.menu == "passwd" && !(E.menu == "passwd_result"))
        {
            if (PasswordParams.selected_param == 0 && PasswordParams.nb_passwd > 1)
            {
                PasswordParams.nb_passwd--;
            }
            else if (PasswordParams.selected_param == 1 && PasswordParams.nb_chars > 4)
            {
                PasswordParams.nb_chars--;
            }
            else if (PasswordParams.selected_param == 2 && PasswordParams.special_idx > 0)
            {
                PasswordParams.special_idx--;
            }
            editorRefreshScreen();
        }
        break;
    case 'D':
        if (E.menu == "passwd" && !(E.menu == "passwd_result"))
        {
            if (PasswordParams.selected_param == 0)
            {
                PasswordParams.nb_passwd++;
            }
            else if (PasswordParams.selected_param == 1)
            {
                PasswordParams.nb_chars++;
            }
            else if (PasswordParams.selected_param == 2 && PasswordParams.special_idx < 3)
            {
                PasswordParams.special_idx++;
            }
            editorRefreshScreen();
        }
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
    abAppend(&ab, "\x1b[?25l", strlen("\x1b[?25l"));

    /* On efface l'écran avec la commande J (Erase in Display). Le 2 spécifie d'effacer tout l'écran.
    On pourrait également effacer du haut jusqu'au curseur avec 1, ou du curseur jusqu'au bas avec 0.
    Attention car ici le curseur est replacé à la fin. */
    abAppend(&ab, "\x1b[2J", 4);

    /* On repositionne le curseur au bon endroit avec la commande H (Cursor Position).
    La commande H prend 2 arguments: le numéro de ligne et le numéro de colonne.
    Par défaut, le numéro de ligne est 1 et le numéro de colonne est 1 (on laisse donc par défaut,
    mais on pourrait mettre \x1b[1;1H si on le souhaitait)
    */
    abAppend(&ab, "\x1b[H", 3);

    // On affiche la fenêtre actuelle
    // TODO: Utiliser un enum pour définir la fenêtre actuelle
    if (E.menu == "main")
    {
        print_main_menu(&ab);
    }
    else if (E.menu == "passwd")
    {
        print_passwd_menu(&ab);
    }
    else if (E.menu == "passwd_result")
    {
        print_passwd_result(&ab);
        PasswordParams.generate = true;
    }
    else if (E.menu == "passphrase")
    {
        // TODO: Fonction pour afficher le menu de génération de phrase de passe
    }
    else if (E.menu == "strenght")
    {
        // TODO: Fonction pour afficher le menu de test de force du mot de passe
    }

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
    int nb_spaces = (E.screencols - (strlen(SB_LEFT) + strlen(SB_MIDDLE) + strlen(SB_RIGHT))) / 2;

    char *spaces = malloc(sizeof(char) * nb_spaces + 1);
    spaces[nb_spaces] = '\0';
    memset(spaces, ' ', nb_spaces);

    char *args[5] = {SB_LEFT, spaces, SB_MIDDLE, spaces, SB_RIGHT};
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
    abAppend(ab, "\r\n\r\n", strlen("\r\n\r\n"));
    
    for (int i = 0; i < 3; i++)
    {
        abAppend(ab, menu[i], strlen(menu[i]));
    }

    // Calcule les espaces entre les modes
    char *mode[2] = {MODE1, MODE2};
    int center_modes = (E.screencols - strlen(mode[0]) - 3) / 2;

    char *spaces = (char *) malloc(sizeof(char) * center_modes + 1);
    spaces[center_modes] = '\0';
    memset(spaces, ' ', center_modes);

    char *line1_args[1] = {spaces};
    
    char *line1 = format_str(mode[0], line1_args, 1, '~');
    char *line2 = format_str(mode[1], line1_args, 1, '~');

    // On peut afficher
    abAppend(ab, line1, strlen(line1));
    abAppend(ab, line2, strlen(line2));
    
    // On affiche la status bar
    print_statusbar(ab);

    // On repositionne le curseur en haut à gauche
    abAppend(ab, "\x1b[H", strlen("\x1b[H"));

    // On libère la mémoire allouée
    free(spaces);
    free(line1);
    free(line2);
}


void print_passwd_menu(struct abuf *ab)
{

    char *special_chars[3] = {"!@#$%^&*()_+-=[]{}|;':,./<>?", "@&$!#?-", "None"};
    PasswordParams.special_chars = special_chars[PasswordParams.special_idx];

    // On affiche le titre
    print_title(ab);

    // On saute une ligne
    abAppend(ab, "\r\n", strlen("\r\n"));

    // On calcule le décalage pour le premier message
    int nb_spaces_welcome = (E.screencols - strlen(PASSWD_WELCOME)) / 2;

    // Idem pour les autres
    int nb_spaces_other = (E.screencols - strlen(PASSWD_EXPLAIN1)) / 2;

    // On alloue la mémoire pour les espaces
    char *spaces_welcome = (char *) malloc(sizeof(char) * nb_spaces_welcome + 1);
    spaces_welcome[nb_spaces_welcome] = '\0';
    memset(spaces_welcome, ' ', nb_spaces_welcome);

    char *spaces_other = (char *) malloc(sizeof(char) * nb_spaces_other + 1);
    spaces_other[nb_spaces_other] = '\0';
    memset(spaces_other, ' ', nb_spaces_other);

    // On insère les espaces dans les messages
    char *welcome = format_str(PASSWD_WELCOME, &spaces_welcome, 1, '~');
    char *explain1 = format_str(PASSWD_EXPLAIN1, &spaces_other, 1, '~');
    char *explain2 = format_str(PASSWD_EXPLAIN2, &spaces_other, 1, '~');
    char *explain3 = format_str(PASSWD_EXPLAIN3, &spaces_other, 1, '~');

    // On affiche les messages
    abAppend(ab, welcome, strlen(welcome));
    abAppend(ab, explain1, strlen(explain1));
    abAppend(ab, explain2, strlen(explain2));
    abAppend(ab, explain3, strlen(explain3));


    // Idem pour les paramètres en rajoutant les infos
    char *templates[3] = {PASSWD_PARAM1, PASSWD_PARAM2, PASSWD_PARAM3};
    char *infos[3] = {int2str(PasswordParams.nb_passwd), int2str(PasswordParams.nb_chars), PasswordParams.special_chars};

    for (int i = 0; i < 3; i++)
    {
        char *selection = "\0";
        if (i == PasswordParams.selected_param)
        {
            selection = "\033[96m> \033[0m";
        }
        char *args_param[3] = {spaces_other, selection, infos[i]};

        char *tmp = format_str(templates[i], args_param, 3, '~');
        abAppend(ab, tmp, strlen(tmp));

        // On libère la mémoire allouée par format_str()
        free(tmp);
    }

    // On modifie le message du milieu de la statusbar
    char *pluriel = "s ";
    if (PasswordParams.nb_passwd == 1) pluriel = " ";

    char *sb_args[3] = {int2str(PasswordParams.nb_passwd), pluriel, int2str(PasswordParams.nb_chars)};
    SB_MIDDLE = format_str("~ password~of ~ characters will be generated", sb_args, 3, '~');

    // On affiche la status bar
    print_statusbar(ab);

    // On libère la mémoire allouée dans cette fonction
    free(spaces_welcome);
    free(spaces_other);

    // La mémoire allouée par format_str()
    free(welcome);
    free(explain1);
    free(explain2);
    free(explain3);
    free(SB_MIDDLE);

    // La mémoire allouée par int2str()
    free(infos[0]);
    free(infos[1]);
    free(sb_args[0]);
    free(sb_args[2]);
}


void print_passwd_result(struct abuf *ab)
{
    // On affiche le titre
    print_title(ab);

    // On saute une ligne
    abAppend(ab, "\r\n", strlen("\r\n"));


    // Si on affiche pour la première fois
    if (!E.lock)
    {
        free_passwd_buffer();
        // On génère les mdp
        PasswordParams.passwords = malloc(sizeof(char *) * PasswordParams.nb_passwd);
        clock_t start = clock();
        generate_passwd(PasswordParams.nb_passwd, PasswordParams.nb_chars, PasswordParams.passwords, PasswordParams.special_chars);
        PasswordParams.exec_time = (double) (clock() - start) / CLOCKS_PER_SEC;
        E.lock = true;
    }
    

    // Si pas assez de place pour afficher tous les mdp, on les met dans un fichier
    if (PasswordParams.nb_passwd > 8 + 3 + 1 + 5 || PasswordParams.nb_chars > E.screencols - 2)
    {
        // TODO: Ecrire dans fichier
        return;
    }

    // On affiche un message
    char *message = "~Here are your passwords, press ctrl + g to generate again !\r\n\r\n";

    int center_message = (E.screencols - strlen(message)) / 2;
    char *message_spaces = (char *) malloc(sizeof(char) * center_message + 1);
    message_spaces[center_message] = '\0';
    memset(message_spaces, ' ', center_message);

    char *message_formatted = format_str(message, &message_spaces, 1, '~');
    abAppend(ab, message_formatted, strlen(message_formatted));


    // On calcule le décalage pour centrer les mdp
    int nb_spaces = (E.screencols - PasswordParams.nb_chars) / 2;

    // On alloue la mémoire pour les espaces
    char *spaces = (char *) malloc(sizeof(char) * nb_spaces + 1);
    spaces[nb_spaces] = '\0';
    memset(spaces, ' ', nb_spaces);

    // On affiche les mdp
    for (int i = 0; i < PasswordParams.nb_passwd; i++)
    {
        char *args[3] = {spaces, PasswordParams.passwords[i], "\r\n"};
        char *tmp = format_str("~~~", args, 3, '~');
        abAppend(ab, tmp, strlen(tmp));
        free(tmp);
    }

    // On affiche la status bar
    char *pluriel = "s ";
    if (PasswordParams.nb_passwd == 1) pluriel = " ";

    char *time_passed = (char *) malloc(sizeof(char) * 6);
    time_passed[5] = '\0';
    sprintf(time_passed, "%.3f", PasswordParams.exec_time);

    char *sb_args[4] = {int2str(PasswordParams.nb_passwd), pluriel, int2str(PasswordParams.nb_chars), time_passed};

    SB_MIDDLE = format_str("~ password~of ~ characters generated in ~s", sb_args, 4, '~');
    print_statusbar(ab);

    // On libère la mémoire allouée dans cette fonction
    free(message_spaces);
    free(spaces);
    free(time_passed);

    // La mémoire allouée par format_str()
    free(message_formatted);
    free(SB_MIDDLE);

    // La mémoire allouée par int2str()
    free(sb_args[0]);
    free(sb_args[2]);
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
        if (nread == -1 && errno != EAGAIN) die("Unable to read from stdin");
    }

    if (c == '\x1b')
    {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[')
        {
            switch (seq[1])
            {
                case 'A': return 'Z';
                case 'B': return 'S';
                case 'C': return 'D';
                case 'D': return 'Q';
            }
        }
        return '\x1b';
    }

    return c;
}


void free_passwd_buffer()
{
    if (PasswordParams.passwords != NULL)
    {
        for (int i = 0; i < PasswordParams.nb_passwd; i++)
        {
            free(PasswordParams.passwords[i]);
        }
        free(PasswordParams.passwords);
    }
}