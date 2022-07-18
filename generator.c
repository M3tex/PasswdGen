/* 
Ce programme permet de générer des mots de passe, en utilisant
des fonctions compatibles avec un usage cryptographique.

Ajouter une interface + accueillante
TODO: Possibilité de choisir des paternes de mots de passe
TODO: Possibilité de choisir les caractères spéciaux
TODO: Possiblité de copier le mot de passe dans le presse-papier 
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
void generate_passwd(int nb, int len, char *passwd[]);
void generate_passphrase(int passwd_nb, int words_nb);
void csprng_shuffle(char *str);
void print_title();






/*** Variables Globales ***/

char *g_alphabet = "abcdefghijklmnopqrstuvwxyz";
char *g_alphabet_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *g_alphabet_numeric = "0123456789";
char *g_alphabet_special = "!#$%&()*+-/<>=?@^|~";



#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif





/*** Visuel ***/

/**
 * @brief Permet d'effacer le terminal
 * 
 */
void clear_screen()
{
    system(CLEAR);
}

void print_title()
{
    printf("#######\\                                        ######\\                      \n");
    printf("##  __##\\                                      ##  __##\\                    \n");
    printf("## |  ## |######\\   #######\\  #######\\         ## /  \\__| ######\\  #######\\  \n");
    printf("#######  |\\____##\\ ##  _____|##  _____|######\\ ## |####\\ ##  __##\\ ##  __##\\ \n");
    printf("##  ____/ ####### |\\######\\  \\######\\  \\______|## |\\_## |######## |## |  ## |\n");
    printf("## |     ##  __## | \\____##\\  \\____##\\         ## |  ## |##   ____|## |  ## |\n");
    printf("## |     \\####### |#######  |#######  |        \\######  |\\#######\\ ## |  ## |\n");
    printf("\\__|      \\_______|\\_______/ \\_______/          \\______/  \\_______|\\__|  \\__|\n");
    printf("\n");
    printf("\n");
}





/*** Fonctions Utilitaires ***/

/**
 * @brief Convertit un string en entier si possible.
 * 
 * @param input Le string à convertir
 * @param result Le pointeur vers la variable où l'on souhaite stocker l'entier
 * @return 0 si le string a été converti avec succès, -1 sinon
 */
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


/**
 * @brief Permet d'effacer les caractères saisis par l'utilisateur
 * et non lus par fgets().
 * 
 */
void clear_input()
{
    // On efface les caractères superflus saisis 
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Permet de vérifier que le résultat de x + y ne produit
 * pas d'int overflow. Dans le cas où l'addition est possible
 * on la réalise.
 * 
 * @param x 
 * @param y 
 * @param result Le pointeur vers la variable où l'on souhaite stocker
 * le résultat de x + y
 * @return 0 si l'addition est possible, -1 sinon
 */
int add_int(int x, int y, int *result)
{
    // On vérifie qu'il n'y a pas d'int overflow
    if (x > INT_MAX - y)
    {
        return -1;
    }
    else 
    {
        *result = x + y;
        return 0;
    }
}


/**
 * @brief Permet de vérifier que le résultat de x * y ne produit
 * pas d'int overflow. Dans le cas où la multiplication est possible
 * on la réalise.
 * 
 * @param x 
 * @param y 
 * @param result Le pointeur vers la variable où l'on souhaite stocker
 * le résultat de x * y
 * @return 0 si la multiplication est possible, -1 sinon
 */
int mult_int(int x, int y, int *result)
{
    // On vérifie qu'il n'y a pas d'int overflow
    if (y != 0 && x > INT_MAX / y)
    {
        return -1;
    }
    else 
    {
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
    // Le buffer est de 10 car l'entier max en C est inférieur à 10 ^ 10 (et + 1 pour le '\0')
    int buff = 10 + 1;
    char input[buff];
    int result;
    
    while (1)
    {
        printf("%s", msg);
        fgets(input, buff, stdin);

        // On vérifie que l'entrée ne dépassera pas la longueur max
        for (int i = 0; i < buff; i++)
        {
            if (input[i] == '\n')
            {
                // Dans ce cas l'entrée est inférieure à la longueur max
                break;
            } 
            else if (i == buff - 1)
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


/**
 * @brief Permet de générer nb mots de passes de longueur len.
 * 
 * @param nb Le nombre de mots de passes à générer.
 * @param len La longueur des mots de passes
 * @param passwd Le tableau où on stockera les mots de passes
 */
void generate_passwd(int nb, int len, char *passwd[])
{
    // On initialise notre tableau d'alphabets
    char *alphabets[4] = {g_alphabet, g_alphabet_upper, g_alphabet_numeric, g_alphabet_special};
    
    for (int i = 0; i < nb; i++)
    {
        // Pour chaque mot de passe, on alloue de la mémoire
        passwd[i] = (char *) malloc(sizeof(char) * (len + 1));
        if (passwd[i] == NULL)
        {
            printf("Erreur d'allocation mémoire.\n");
            return;
        }

        passwd[i][len] = '\0';

        // On force la présence d'au moins chaque type de caractère (on mélangera après)
        for (int n = 0; n < 4; n++)
        {
            passwd[i][n] = alphabets[n][randombytes_uniform(strlen(alphabets[n]))];
        }

        // On remplit le reste en tirant des caractères aléatoires
        for (int j = 4; j < len; j++)
        {
            // On choisit un type de caractère aléatoire
            int rand_alph = randombytes_uniform(4);
            
            // On peut maintenant sélectionner un caractère aléatoire
            int rand_char = randombytes_uniform(strlen(alphabets[rand_alph]));
            passwd[i][j] = alphabets[rand_alph][rand_char];
        }

        // On mélange les caractères
        csprng_shuffle(passwd[i]);
    }
}



void generate_passphrase(int passwd_nb, int words_nb)
{
    // TODO: Supporter plusieurs langues ?
    // On ouvre le dictionnaire
    FILE *file = fopen("/home/mathis/Programmation/C/PasswdGen/dico.txt", "r");
    if (file == NULL)
    {
        printf("Impossible d'ouvrir le fichier.\n");
        return;
    }
    
    // On récupère le nombre de mots du dictionnaire
    int dico_size = 1;  // Init à 1 pour compter le premier mot
    while (!feof(file))
    {
        char tmp = fgetc(file);
        if (tmp == '\n')
        {
            dico_size++;
        }
    }
    fseek(file, 0, SEEK_SET);

    /* On charge le dictionnaire en mémoire.
    On utilisera seulement un tableau car on a pas besoin de rechercher des mots
    à l'intérieur du dictionnaire mais seulement d'en sélectionner au hasard. 
    (donc pas besoin de structure + complexe qu'un tableau) */
    char *dictio[dico_size];
    
    /* On va lire chaque ligne (=mot) du fichier et les stocker dans le tableau
    Pour cela on commence par lire les lignes et déterminer la taille des mots 
    puis on alloue la mémoire nécessaire */
    int i = 0;
    int len = 0;
    int pos = 0;
    while (1)
    {
        char tmp = fgetc(file);
        len++;
        if (tmp == '\n')
        {
            // On a trouvé un mot
            dictio[i] = (char *) malloc(sizeof(char) * (len));
            if (dictio[i] == NULL)
            {
                printf("Erreur d'allocation mémoire.\n");
                return;
            }

            dictio[i][len - 1] = '\0';

            fseek(file, pos, SEEK_SET);
            fgets(dictio[i], len, file);
            fgetc(file);    // On skip le '\n'

            len = 0;
            i++;
            pos = ftell(file);
        }
        else if (tmp == EOF)
        {
            // On a atteint la fin du fichier
            break;
        }
        //if (i > 6) return;
    }

    // On peut maintenant générer une passphrase
    printf("\n\nPassphrase : \n");
    for (int i = 0; i < passwd_nb; i++)
    {
        int words_pos[words_nb];
        int sum_len = 0;
        for (int j = 0; j < words_nb; j++)
        {
            int rand_word = randombytes_uniform(dico_size);
            int len = strlen(dictio[rand_word]);
            
            words_pos[j] = rand_word;
            sum_len += len;
            if (j != words_nb - 1)
            {
                // On prend en compte le '-'
                sum_len++;
            }
        }

        // On peut maintenant allouer la mémoire et concatener les mots
        char *passphrase = (char *) malloc(sizeof(char) * (sum_len + 1));
        if (passphrase == NULL)
        {
            printf("Erreur d'allocation mémoire.\n");
            return;
        }

        passphrase[sum_len] = '\0';

        for (int j = 0; j < words_nb; j++)
        {
            strcat(passphrase, dictio[words_pos[j]]);
            if (j != words_nb - 1)
            {
                strcat(passphrase, "-");
            }
        }
        printf("%s\n", passphrase);
        free(passphrase);
    }

    // On peut maintenant libérer la mémoire allouée
    for (int i = 0; i < dico_size; i++)
    {
        free(dictio[i]);
    }

    fclose(file);
}


/**
 * @brief Permet de mélanger un string en utilisant l'algorithme de Fisher-Yates.
 * 
 * @param str Le string à mélanger.
 */
void csprng_shuffle(char *str)
{
    for (int i = strlen(str) - 1; i > 0; i--)
    {
        int j = randombytes_uniform(i + 1);
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
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
    clear_screen();
    print_title();

    // On demande à l'utilisateur le nombre de mots de passe à générer
    int nb_passwd = get_int("Combien de mots de passe voulez-vous générer ?\n");

    // On demande à l'utilisateur la longueur de chaque mot de passe
    int passwd_len = get_int("Combien de caractères voulez-vous avoir pour chaque mot de passe ?\n");
    while (passwd_len < 4)
    {
        printf("Le mot de passe doit contenir au moins 4 caractères pour satisfaire les conditions !\n");
        passwd_len = get_int("Combien de caractères voulez-vous avoir pour chaque mot de passe ?\n");
    }
    
    clear_screen();
    print_title();

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

        // On libère la mémoire allouée pour les mots de passes
        free(passwords[i]);
    }

    generate_passphrase(1, 5);

    printf("\n%d mots de passes de %d caractères ont été générés en %fs\n", nb_passwd, passwd_len, tps);
    printf("Appuyez sur entrée pour quitter...  ");
    while(1)
    {
        if (getchar())
        {
            clear_screen();
            break;
        }
    }
}
