#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT "exit"
#define HELP "help"
#define CLEAR "clear"
#define CD "cd"
#define PS "ps"

#define BUFFER 1024

/**
* Shell programs
*/

void help();
void clear();
void ps();
void cd(char *args[]);
int execute(char *args[]);

/**
* Helpers
*/

const char *path();
const char *user();
void parse_args(char *args[], char command[], int *args_count);

int main()
{
    clear();
    while (true)
    {

        char *args[BUFFER];
        int args_count = 0;
        char command[BUFFER];

        printf("[%s:%s] \n$ ", user(), path());
        fgets(command, sizeof command, stdin);
        parse_args(args, command, &args_count);

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
        else if (strcmp(args[0], CD) == 0)
        {
            cd(args);
        }
        else if (strcmp(args[0], PS) == 0)
        {
            ps();
        }
        else
        {
            if (execute(args) < 0)
            {
                printf("%s, type help if you got lost. \n", strerror(errno));
            }
        }
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
        perror("cannot get username");
        exit(errno);
    }

    return pw->pw_name;
}

const char *path()
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

void parse_args(char *args[], char command[], int *args_count)
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
    *args_count = i - 1;
}

/**
* Shell programs
*/

void ps()
{
    DIR *dir;
    FILE *file;
    regex_t regex;
    struct dirent *entry;
    char procbuf[BUFFER];
    int regex_status;

    regex_status = regcomp(&regex, "^[0-9]*$", 0);

    if (regex_status)
    {
        exit(errno);
    }

    if ((dir = opendir("/proc/")) == NULL)
    {
        perror("/proc/ error");
        exit(errno);
    }
    else
    {
        printf("%-9s %s \n", "pid", "cmdline");
        while ((entry = readdir(dir)) != NULL)
        {
            regex_status = regexec(&regex, entry->d_name, 0, NULL, 0);
            if (!regex_status)
            {
                char pid[BUFFER], cmdline[BUFFER];
                strcpy(pid, entry->d_name);
                strcpy(procbuf, "/proc/");

                strcat(procbuf, entry->d_name);
                strcat(procbuf, "/cmdline");

                file = fopen(procbuf, "r");
                fgets(cmdline, BUFFER, file);
                printf("%-9s %s \n", pid, cmdline);
            }
        }
        closedir(dir);
    }
    regfree(&regex);
}

void clear()
{
    printf("\e[1;1H\e[2J");
}

void cd(char *args[])
{
    if (args[1] == NULL)
    {
        chdir(".");
    }
    else if (strcmp(args[1], "..") == 0)
    {
        chdir("..");
    }
    else if (strcmp(args[1], "~") == 0)
    {
        chdir(getenv("HOME"));
    }
    else if (chdir(args[1]) == -1)
    {
        printf("Fatal error: %s\n", strerror(errno));
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
    printf("cd     \t\tThere will be a cool info.\n");
    printf("\n\n");
}