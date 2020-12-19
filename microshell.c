#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT "exit"
#define HELP "help"
#define CLEAR "clear"

#define BUFFER 256

void help();
void clear();

const char *get_current_path();
void parse_args(char *args[], char command[]);

int main()
{

    while (true)
    {
        char *args[BUFFER];
        char command[BUFFER];

        printf("[%s] \n$ ", get_current_path());
        fgets(command, sizeof command, stdin);
        parse_args(args, command);

        if (strcmp(args[0], EXIT) == 0)
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
        else
        {
            printf("Unknown command, type help if you got lost.\n");
        }
    }

    exit(EXIT_SUCCESS);
}

const char *get_current_path()
{
    static char current_path[BUFFER];
    getcwd(current_path, sizeof current_path);
    if (current_path == NULL)
    {
        perror("cannot get current path");
        exit(errno);
    }
    return current_path;
}

void parse_args(char *args[], char command[])
{
    int i = 0;
    char *temp;
    temp = strtok(command, " \n\t");
    while (temp != NULL)
    {
        args[i++] = temp;
        temp = strtok(NULL, " \n\t");
    }
    args[i] = NULL;
}

void clear()
{
    printf("\e[1;1H\e[2J");
}

void help()
{
    printf("\e[1;1H\e[2J");
    printf("\n");
    printf(" /$$$$$$$   /$$$$$$  /$$   /$$ /$$$$$$$$ /$$       /$$      \n");
    printf("| $$__  $$ /$$__  $$| $$  | $$| $$_____/| $$      | $$      \n");
    printf("| $$  \\ $$| $$  \\__/| $$  | $$| $$      | $$      | $$      \n");
    printf("| $$  | $$|  $$$$$$ | $$$$$$$$| $$$$$   | $$      | $$      \n");
    printf("| $$  | $$ \\____  $$| $$__  $$| $$__/   | $$      | $$      \n");
    printf("| $$  | $$ /$$  \\ $$| $$  | $$| $$      | $$      | $$      \n");
    printf("| $$  | $$ /$$  \\ $$| $$  | $$| $$      | $$      | $$      \n");
    printf("| $$$$$$$/|  $$$$$$/| $$  | $$| $$$$$$$$| $$$$$$$$| $$$$$$$$\n");
    printf("|_______/  \\______/ |__/  |__/|________/|________/|________/\n");
    printf("\n\n");
    printf("Developed by Dawid Korzepa © 2021\n\n");
    printf("clear  \t\tThere will be a cool info.\n");
    printf("help   \t\tThere will be a cool info.\n");
    printf("exit   \t\tThere will be a cool info.\n");
    printf("\n\n");
}