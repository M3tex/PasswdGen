// Header file for gen.c

int get_int(char *msg);
int str2int(char *input, int *result);
int add_int(int x, int y, int *result);
int mult_int(int x, int y, int *result);

char *format_str(char *to_format, char *args[], int argc, char placeholder);
char *int2str(int x);

void clear_input();
void die(char *msg);
void csprng_shuffle(char *str);
void generate_passwd(int nb, int len, char *passwd[], char *special_chars);
void generate_passphrase(int passwd_nb, int words_nb);