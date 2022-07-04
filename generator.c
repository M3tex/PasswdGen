/*
Ce programme permet de générer des mots de passe
*/

/*** Include ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sodium.h>
#include <time.h>


/*** Prototypes ***/
int str2int(char *input, int *result);
int get_int(char *msg);
int add_int(int x, int y, int *result);
int mult_int(int x, int y, int *result);
void clear_input();
void clear_screen();



/*** Variables Globales ***/
int MAX_PASSWD = 6;     // Le nombre max de mdp à générer est 10 ^ MAX_PASSWD
int MAX_PASSWD_LEN = 10; // La longueur max d'un mdp est 10 ^ MAX_PASSWD_LEN

char *alphabet = "abcdefghijklmnopqrstuvwxyz";
char *alphabet_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *alphabet_numeric = "0123456789";
char *alphabet_special = "!#$%&()*+-/<>=?@^|~";



#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif




/*** Visuel ***/
void clear_screen()
{
    system(CLEAR);
}


/*** Fonctions Utilitaires ***/

int str2int(char *input, int *result)
{
    *result = 0;
    while (1)
    {
        if (*input == '\0')
        {
            return 0;
        }
        else if (!isdigit(*input))
        {
            if (*input == '\n' && *result != 0)
            {
                return 0;
            }
            return -1;
        }

        if (mult_int(*result, 10, result) == -1)
        {
            printf("Vous avez entré un entier trop grand\n");
            return -1;
        }
        
        if (add_int(*result, *input - '0', result) == -1)
        {
            printf("Vous avez entré un entier trop grand\n");
            return -1;
        }
        input++;
    }
}

void clear_input()
{
    // On efface les caractères entrés superflus
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


int add_int(int x, int y, int *result)
{
    // On vérifie qu'il n'y a pas d'int overflow
    if (x > INT_MAX - y)
        return -1;
    else {
        *result = x + y;
        return 0;
    }
}


int mult_int(int x, int y, int *result)
{
    // On vérifie qu'il n'y a pas d'int overflow
    if (x != 0 && x > INT_MAX / y)
        return -1;
    else {
        *result = x * y;
        return 0;
    }
}

/**
 * @brief Permet de demander à l'utilisateur un entier positif.
 * La fonction redemandera l'entier jusqu'à ce que l'utilisateur entre un entier positif.
 * 
 * @param msg Le message affiché à l'utilisateur.
 * @param max_len La longueur max de l'entier.
 * @return L'entier entré par l'utilisateur.
 */
int get_int(char *msg)
{
    // Le buffer est de 10 car l'entier max en C est inférieur à 10 ^ 10
    char input[10];
    int result;
    
    while (1)
    {
        printf("%s", msg);
        fgets(input, 11, stdin);

        // On vérifie que l'entrée ne dépassera pas la longueur max
        for (int i = 0; i < 10; i++)
        {
            if (input[i] == '\n')
            {
                // Dans ce cas l'entrée est inférieure à la longueur max
                break;
            } 
            else if (i == 9)
            {
                /* Si on arrive ici c'est qu'on a atteint la longueur max
                On supprime alors les caractères non lus */
                clear_input();
            }
        }

        if (str2int(input, &result) != -1)
        {
            return result;
        }
    }
}


void generate_passwd(int nb, int len, char *passwd[])
{
    // On initialise notre tableau d'alphabets
    char *alphabets[4] = {alphabet, alphabet_upper, alphabet_numeric, alphabet_special};
    
    for (int i = 0; i < nb; i++)
    {
        // Pour chaque mot de passe, on alloue de la mémoire
        passwd[i] = (char *) malloc(sizeof(char) * (len + 1));
        passwd[i][len] = '\0';
        for (int j = 0; j < len; j++)
        {
            // On choisit un type de caractère aléatoire
            int rand_alph = randombytes_uniform(4);
            
            // On peut maintenant sélectionner un caractère aléatoire
            int rand_char = randombytes_uniform(strlen(alphabets[rand_alph]));
            passwd[i][j] = alphabets[rand_alph][rand_char];
        }
    }
}


/*** Main ***/

int main(void)
{
    // On vérifie que sodium est bien initialisé
    if (sodium_init() == -1)
    {
        printf("Erreur lors de l'initialisation de sodium\n");
        return -1;
    }

    // On demande à l'utilisateur le nombre de mots de passe à générer
    int nb_passwd = get_int("Combien de mots de passe voulez-vous générer ?\n");

    // On demande à l'utilisateur la longueur de chaque mot de passe
    int passwd_len = get_int("Combien de caractères voulez-vous avoir pour chaque mot de passe ?\n");
    clear_screen();

    clock_t exec_time = clock();

    // On peut maintenant générer les mots de passes
    char *passwords[nb_passwd];
    generate_passwd(nb_passwd, passwd_len, passwords);

    exec_time = clock() - exec_time;
    double tps = (double) exec_time / CLOCKS_PER_SEC;

    printf("Voici vos mots de passes :\n\n");
    for (int i = 0; i < nb_passwd; i++)
    {
        printf("%s\n", passwords[i]);
    }

    // On libère la mémoire allouée pour les mots de passes
    for (int i = 0; i < nb_passwd; i++)
    {
        free(passwords[i]);
    }

    printf("\n%d mots de passes de %d caractères ont été générés en %fs\n", nb_passwd, passwd_len, tps);
    printf("Appuyez sur entrée pour quitter... ");
    while(1)
    {
        if (getchar())
        {
            clear_screen();
            break;
        }
    }
}



