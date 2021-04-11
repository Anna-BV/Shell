#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

struct Commands {
    char **mas; // указатель на массив слов
    int size_of_bufer; //  размер буфера
    int length_of_mas; // длина
    int vertical_bar; // | конвейер
    char *input_file_name;
    char *output_file_name;
    int pere;
    int is_append; // флаг для перенаправления
} typedef Commands;

struct Pipeline {
    Commands *com;
    int size_of_bufer;
    int length_of_mas;
    int end_bar; // признак конца конвейера
    int ampersand;
}  typedef Pipeline ;

struct  Background_mode {  // фоновый режим
    Pipeline *conv;
    int size_of_bufer;
    int length_of_mas;
} typedef Background_mode;

// Функция для приглашения к вводу команды
void Prig() {
    char buf[PATH_MAX];
    char *name = NULL;
    name = getlogin(); // возвращает указатель на строку, содержащую имя пользователя
    getcwd(buf, PATH_MAX); // путь к текущему рабочему каталогу в массиве
    printf("%s:%s$ ", name, buf);
}

// функция REALLOC для слова команды
void ReallocWord(char **word, int *length_of_word) {
    char *copy_of_word = (char *) malloc(2 * (*length_of_word) * sizeof(char));
    for (int i = 0; i < *length_of_word; i++) {
        copy_of_word[i] = (*word)[i];
    }
    free(*word);
    *word = copy_of_word;
    *length_of_word *= 2;
}
// функция REALLOC  для команды
void ReallocCommand(Commands *command) {
    command->size_of_bufer *= 2;
    char **copy_of_mas = (char **) malloc(command->size_of_bufer * sizeof(char *));
    for (int i = 0; i < command->length_of_mas; i++) {
        *(copy_of_mas + i) = command->mas[i];
    }
    free(command->mas);
    command->mas = copy_of_mas;
}

// функция REALLOC для конвейера
void ReallocPipeline(Pipeline *Conv) {
    Conv->size_of_bufer *= 2;
    Commands *copy = (Commands *) malloc(Conv->size_of_bufer * (sizeof(Commands)));
    for (int i = 0; i < Conv->length_of_mas; i++) {
        *(copy + i) = Conv->com[i];
    }
    free(Conv->com);
    Conv->com = copy;
}

// функция REALLOC для фонового режима
void ReallocBackgroundMode (Background_mode *fon){
    fon->size_of_bufer *= 2;
    Pipeline *mas= (Pipeline*) malloc(fon->size_of_bufer * sizeof(Pipeline));
    for (int i = 0; i < fon->length_of_mas; i++){
        *(mas+i) = fon->conv[i];
    }
    free (fon->conv);
    fon->conv = mas;
}
void ReadInSingleQuotes(char **word, int *length_of_word, int *i){
    int symbol = 0;
    while ((symbol = getchar()) != EOF && symbol != '\'') {
        if (*length_of_word <= *i) {
            ReallocWord(word,length_of_word);
        }
        (*word)[*i] = (char) symbol;
        (*i)++;
    }
}
void ReadInDoubleQuotes(char **word, int *length_of_word, int *i) {
    int symbol = 0;
    while ((symbol = getchar()) != EOF && symbol != '"') {
        if (*length_of_word <= *i) {
            ReallocWord(word, length_of_word);
        }
        (*word)[*i] = (char)symbol;
        (*i)++;
    }
}
void DoCommand(Commands cm, int fd0, int fd1, int ampersand) {
    if (strcmp(cm.mas[0], "cd") != 0) {
        pid_t child = fork();
        if (child < 0) {
            puts("Error in fork()");
        } else {
            if (child == 0) {
                dup2(fd0, 0);
                dup2(fd1, 1);
                execvp(cm.mas[0], cm.mas);
                printf("%s: command not found\n", cm.mas[0]);
                exit(1);
            } else {
                if (fd1 == 1) {
                    if (!ampersand) {
                        wait(NULL);
                    }
                }
            }
        }
    }
}

void DoPipeline(Pipeline c) {
    if (strcmp("cd",c.com[0].mas[0]) == 0 && c.length_of_mas == 1) { // выполнение команды CD
        if (c.com->mas[1] == NULL) {
            puts("cd: too few arguments");
        } else {
            if (c.com->mas[2] != NULL) {
                puts("cd: too many arguments");
            } else {
                if ((chdir(c.com->mas[1])) == -1) {
                    printf("cd: %s: no such file or directory\n", c.com->mas[1]);
                }
            }
        }
    } else {
        pid_t  child = fork();
        if (child == 0) {
            int fd[c.length_of_mas - 1][2]; // канал
            for (int i = 0; i < c.length_of_mas; i++) {
                int fd0 = 0, fd1 = 1;
                if (i < c.length_of_mas - 1) {
                    pipe(fd[i]);
                }
                if (c.com[i].pere) {
                    if (c.com[i].input_file_name != NULL) {
                        fd0 = open (c.com[i].input_file_name, O_RDONLY, 0666);
                        if (fd0 == -1) {
                            printf ("ERROR: can't read file %s\n", c.com[i].input_file_name);
                        }
                    }
                    if (c.com[i].output_file_name != NULL) {
                        if (c.com[i].is_append) {
                            fd1 = open (c.com[i].output_file_name, O_WRONLY|O_CREAT|O_APPEND, 0666);
                        } else {
                            fd1 = open(c.com[i].output_file_name, O_WRONLY|O_TRUNC|O_CREAT, 0666);
                        }
                        if (fd1 == -1) {
                            printf ("ERROR: can't read file %s\n", c.com[i].input_file_name);
                        }
                    }
                } else {
                    if (i == 0) {
                        if (c.length_of_mas != 1) {
                            fd1 = fd[i][1];
                        }
                    } else {
                        if (i == c.length_of_mas - 1) {
                            fd0 = fd[i-1][0];
                        } else {
                            fd0 = fd[i-1][0];
                            fd1 = fd[i][1];
                        }
                    }
                }
                DoCommand(c.com[i], fd0, fd1, c.ampersand);
                if (c.com[i].pere) {
                    if (fd0 != 0){
                        close(fd0);
                    }
                    if (fd1 != 1) {
                        close(fd1);
                    }
                }
                if (i < c.length_of_mas - 1 && i != 0) {
                    close(fd[i - 1][0]);
                }
                close(fd[i][1]);
            }
            exit(1);
        } else {
            waitpid(child,NULL,0);
        }
    }
}

char InputWord (char **str){ // возвращает последний считанный символ
    int c;
    int size = 8;
    int i = 0;
    *str = (char *) malloc (size * sizeof(char));
    while ((c = getchar()) != EOF && c == ' ') ;
    while (c != ' ' && c != '\n') {
        if (i >= size) {
            ReallocWord (str, &size);
        }
        (*str)[i] = (char) c;
        i++;
        c = getchar();
    }
    (*str)[i] = '\0';
    return (char)c;
}

int Vvod(char **a,int *flag_of_end,int *end_com,int *vertical_bar, int *end_bar, int *ampersand, char **out, char **in, int *append, int *pere) {
    int size_of_mas = 8;
    int symbol = 0, i = 0;
    *a = (char *) malloc(size_of_mas * sizeof(char));
    while (1) {  // считываем одно слово
        symbol = getchar();
        if (symbol == '\'') {
            ReadInSingleQuotes(a, &size_of_mas, &i);
            continue;
        } else {
            if (symbol == '"') {
                ReadInDoubleQuotes(a, &size_of_mas, &i);
                continue;
            }
        }
        if (symbol == '<') {
            symbol = InputWord(in);
            *pere = 1;
        } else {
            if (symbol == '>') {
                if ((symbol = getchar()) != EOF && symbol == '>') { // >>
                    symbol = InputWord(out);
                    *append = 1;
                } else {
                    symbol = InputWord(out);
                }
                *pere = 1;
            }
        }
        if (symbol == '|') {
            *vertical_bar = 1;
            *end_bar = 1;
            break;
        }
        if (symbol == '&') {
            *ampersand = 1;
            *end_com = 1;
            break;
        }
        if (symbol == ' ' || symbol == '\t') {
            break;
        }
        if (symbol == '\\' && (symbol = getchar()) != EOF && symbol == '\n') {
                continue;
        }
        if (symbol == '\n') {
            if (!*end_bar) {
                *flag_of_end = 1;
                *end_com = 1;
            }
            break;
        }
        if (i >= size_of_mas) {
            ReallocWord(a, &size_of_mas);
        }
        (*a)[i] = (char) symbol;
        i++;
        *end_bar = 0;
    }
    (*a)[i] = '\0';
    return i; // функция возвращает длину слова
}

void Vivod (Commands a){
    for (int i = 0; i < a.length_of_mas; i++){
        printf ("<%s> ", a.mas[i]);
    }
    printf ("\n%s, %s ", a.output_file_name, a.input_file_name);
    printf ("%d\n", a.vertical_bar);
}

int main () {
    while (1) {
        Prig(); // приглашение к вводу команды
        Background_mode fon;
        fon.size_of_bufer = 2;
        fon.length_of_mas = 0;
        fon.conv = (Pipeline *) malloc (fon.size_of_bufer* sizeof(Pipeline));
        int end = 0; // флаг конца команды
        do {
            Pipeline pp;
            pp.size_of_bufer = 2;
            pp.length_of_mas = 0;
            pp.ampersand = 0;
            pp.end_bar = 0;
            pp.com = (Commands *) malloc (pp.size_of_bufer* sizeof(Commands));
            do { // обработка одной команды
                Commands cm;
                cm.size_of_bufer = 8;
                cm.length_of_mas = 0;
                cm.vertical_bar = 0;
                cm.is_append = 0;
                cm.input_file_name = NULL;
                cm.output_file_name = NULL;
                cm.pere = 0;
                cm.mas = (char **) malloc(cm.size_of_bufer * sizeof(char *));
                // InputCommand
                int length; // длина слова
                int end_com = 0;
                do { // чтение одной команды
                    length = Vvod(&cm.mas[cm.length_of_mas], &end, &end_com, &cm.vertical_bar, &pp.end_bar,
                                  &pp.ampersand, &cm.output_file_name, &cm.input_file_name, &cm.is_append,
                                  &cm.pere); // приравнять длине
                    if (length) { //  слово | слово ...
                      cm.length_of_mas++;
                    }

                    if (cm.vertical_bar){  // если встречаем |, то конец команды - читаем следующую
                      break;
                     }
                     if (!length) {
                      continue;
                }
                    if (cm.length_of_mas >= cm.size_of_bufer) { // реалоккк если надо
                        ReallocCommand(&cm);
                    }
                    if (cm.length_of_mas == 1 && strcmp("exit", cm.mas[0]) == 0) { // если введена команда exit
                        break;
                    }
                } while (end_com == 0);

                cm.mas[cm.length_of_mas] = NULL;
                if (cm.length_of_mas == 1 && strcmp("exit", cm.mas[0]) == 0) { // если введена команда exit
                    return 0;
                }

                if (!cm.length_of_mas) {
                    break;
                }
                (pp.com)[pp.length_of_mas] = cm; // добавляем команду в конвейер
                pp.length_of_mas++;
                if (pp.length_of_mas >= pp.size_of_bufer) {
                    ReallocPipeline(&pp);
                }
            } while ((pp.com)[pp.length_of_mas - 1].vertical_bar) ;
            if (!pp.length_of_mas) {
                break;
            }
            (fon.conv)[fon.length_of_mas] = pp;
            fon.length_of_mas++;
            if (fon.length_of_mas >= fon.size_of_bufer){
                ReallocBackgroundMode(&fon);
            }
        } while (!end);
        for (int i = 0; i < fon.length_of_mas; i++) {
            DoPipeline(fon.conv[i]);
            /*for (int j = 0; j < fon.conv[i].length_of_mas; j++) {
                Vivod (fon.conv[i].com[j]);
            }*/
        }
    }
}