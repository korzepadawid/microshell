#include <dirent.h>
#include <errno.h>
#include <history.h>
#include <libgen.h>
#include <pwd.h>
#include <readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT "exit"
#define HELP "help"
#define CLEAR "clear"
#define MOVE "mv"
#define CHANGE_DIR "cd"

#define MAG "\e[0;35m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define HCYN "\e[0;96m"
#define GREY "\x1B[90m"
#define RESET "\x1B[0m"

#define HELP_FORMAT "%-9s %s \n"

#define BUFFER 1024

/**
* Shell programs
*/

void help();
void clear();
void move(char *args[], int args_count);
void change_dir(char *args[], int args_count);
int execute(char *args[]);

/**
* Helpers
*/

const char *path();
const char *user();
char *substring(char *string, int position, int length);
void parse_args(char *args[], char command[], int *args_count);

int main()
{
    clear();
    char *input;
    rl_bind_key('\t', rl_complete);
    while (true)
    {
        char *args[BUFFER];
        int args_count = 0;
        printf("%s[%s%s%s:%s%s%s]%s\n", GREY, MAG, user(), GREY, HCYN, path(), GREY, RESET);
        input = readline("$ ");

        if (input != NULL)
        {
            add_history(input);
        }

        parse_args(args, input, &args_count);

        if (args[0] == NULL)
        {
            continue;
        }
        else if (strcmp(args[0], EXIT) == 0)
        {
            exit(0);
        }
        else if (strcmp(args[0], HELP) == 0)
        {
            help();
        }
        else if (strcmp(args[0], CLEAR) == 0)
        {
            clear();
        }
        else if (strcmp(args[0], MOVE) == 0)
        {
            move(args, args_count);
        }
        else if (strcmp(args[0], CHANGE_DIR) == 0)
        {
            change_dir(args, args_count);
        }
        else
        {
            if (execute(args) < 0)
            {
                fprintf(stderr, RED "%s, type help if you got lost.\n" RESET, strerror(errno));
            }
        }
        free(input);
    }

    exit(EXIT_SUCCESS);
}

/**
* Helpers
*/

const char *user()
{
    register uid_t uid = geteuid();
    register struct passwd *pw = getpwuid(uid);
    if (pw == NULL)
    {
        fprintf(stderr, "cannot get username.\n");
        exit(EXIT_FAILURE);
    }

    return pw->pw_name;
}

const char *path()
{
    static char current_path[BUFFER];
    getcwd(current_path, sizeof current_path);
    if (current_path == NULL)
    {
        fprintf(stderr, "cannot get path.\n");
        exit(EXIT_FAILURE);
    }
    return current_path;
}

char *substring(char *string, int position, int length)
{
    char *p;
    int c;

    p = malloc(length + 1);

    if (p == NULL)
    {
        printf("Unable to allocate memory.\n");
        exit(1);
    }

    for (c = 0; c < length; c++)
    {
        *(p + c) = *(string + position - 1);
        string++;
    }

    *(p + c) = '\0';

    return p;
}

void parse_args(char *args[], char command[], int *args_count)
{
    int i = 0;
    char *temp;
    temp = strtok(command, " \'\n\t");
    while (temp != NULL)
    {
        args[i++] = temp;
        temp = strtok(NULL, " \'\n\t");
    }
    args[i] = NULL;
    *args_count = i;
}

/**
* Shell programs
*/

void move(char *args[], int args_count)
{
    if (args_count != 3)
    {
        fprintf(stderr, RED "Wrong format, use mv <source> <destination>\n" RESET);
        return;
    }

    char *source = args[1], *destination = args[2], destination_path[BUFFER];

    if (destination[0] == '/')
    {
        strcat(destination, "/");
        strcat(destination, source);
        if ((rename(source, destination)) != 0)
        {
            fprintf(stderr, RED "Unknown path\n" RESET);
        }
    }
    else if (strcmp(destination, ".") == 0)
    {
        strcpy(destination_path, path());
        strcat(destination_path, "/");
        strcat(destination_path, basename(source));
        if ((rename(source, destination_path)) != 0)
        {
            fprintf(stderr, RED "Unknown path\n" RESET);
        }
    }
    else
    {
        DIR *dir;
        if ((dir = opendir(destination)) == NULL)
        {
            if ((rename(source, destination)) != 0)
            {
                fprintf(stderr, RED "An error occurred\n" RESET);
            }
        }
        else
        {
            char *current_location = getcwd(destination_path, sizeof(destination_path));
            strcat(destination_path, "/");
            strcat(destination_path, destination);
            strcat(destination_path, "/");
            strcat(destination_path, source);
            if ((rename(source, current_location)) != 0)
            {
                fprintf(stderr, RED "Unknown directory\n" RESET);
            }
        }
        closedir(dir);
    }
}

void clear()
{
    printf("\e[1;1H\e[2J");
}

void change_dir(char *args[], int args_count)
{

    if (args_count > 2)
    {
        fprintf(stderr, RED "Wrong format, use cd <folder>\n" RESET);
        return;
    }
    else if (args[1] == NULL || strcmp(args[1], "~") == 0)
    {
        chdir(getenv("HOME"));
    }
    else if (strcmp(args[1], "..") == 0)
    {
        chdir("..");
    }
    else if (chdir(args[1]) == -1)
    {
        fprintf(stderr, RED "Fatal error %s\n" RESET, strerror(errno));
        return;
    }
    else
    {
        chdir(args[1]);
    }
}

int execute(char *args[])
{
    int pid, result;
    if ((pid = fork()) == 0)
    {
        result = execvp(args[0], args);
    }
    else
    {
        wait(NULL);
    }
    return result;
}

void help()
{
    printf("\e[1;1H\e[2J");
    printf(GRN);
    printf("##################################################################\n");
    printf("#                                                                #\n");
    printf("#                                                                #\n");
    printf("#                                                                #\n");
    printf("#                                                                #\n");
    printf("#                                                                #\n");
    printf("##################################################################\n\n");
    printf(RESET);
    printf(RED "Developed by Dawid Korzepa © 2021\n\n" RESET);
    printf(GRN HELP_FORMAT RESET, "clear", "There will be a cool info.");
    printf(GRN HELP_FORMAT RESET, "help", "There will be a cool info.");
    printf(GRN HELP_FORMAT RESET, "exit", "There will be a cool info.");
    printf(GRN HELP_FORMAT RESET, "cd", "There will be a cool info.");
    printf(GRN HELP_FORMAT RESET, "ps", "There will be a cool info.");
    printf(GRN HELP_FORMAT RESET, "ls", "There will be a cool info.");
    printf("\n\n");
}