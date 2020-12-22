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
#define MV "mv"

#define BUFFER 1024
#define COLUMN_FORMAT "%-9s %s \n"

/**
* Shell programs
*/

void help();
void clear();
void ps();
void mv(char *args[], int args_count);
void cd(char *args[], int args_count);
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

        char command[BUFFER];
        char *args[BUFFER];
        int args_count = 0;
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
        else if (strcmp(args[0], PS) == 0)
        {
            ps();
        }
        else if (strcmp(args[0], CD) == 0)
        {
            cd(args, args_count);
        }
        else if (strcmp(args[0], MV) == 0)
        {
            mv(args, args_count);
        }
        else
        {
            if (execute(args) < 0)
            {
                fprintf(stderr, "%s, type help if you got lost.\n", strerror(errno));
            }
        }
        strcpy(command, "");
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

void mv(char *args[], int args_count)
{
    char *file = args[1];
    char *location = args[2];
    char newplace[BUFFER];

    if (args_count != 2)
    {
        fprintf(stderr, "Wrong format, use mv <target> <destination>\n");
        return;
    }
    else
    {
        if (location[0] == '/')
        {
            strcat(location, "/");
            strcat(location, file);

            if (rename(file, location) != 0)
            {
                fprintf(stderr, "Unknwon destination\n");
                return;
            }
        }
        else
        {
            DIR *isD;
            isD = opendir(location);

            if (isD == NULL && rename(file, location) != 0)
            {
                fprintf(stderr, "Something went wrong...\n");
                return;
            }
            else
            {
                char *ptrL;
                ptrL = getcwd(newplace, 50);
                strcat(newplace, "/");
                strcat(newplace, location);
                strcat(newplace, file);
                if (rename(file, ptrL) == -1)
                {
                    fprintf(stderr, "No such file or directory\n");
                    return;
                }
                closedir(isD);
            }
        }
    }
}

void ps()
{
    DIR *dir;
    FILE *file;
    regex_t regex;
    struct dirent *entry;
    char procbuf[BUFFER];
    int regex_error;

    regex_error = regcomp(&regex, "^[0-9]*$", 0);

    if (regex_error)
    {
        fprintf(stderr, "regex error.\n");
        exit(EXIT_FAILURE);
    }

    if ((dir = opendir("/proc/")) == NULL)
    {
        fprintf(stderr, "/proc/ error \n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(COLUMN_FORMAT, "PID", "CMD");
        while ((entry = readdir(dir)) != NULL)
        {
            regex_error = regexec(&regex, entry->d_name, 0, NULL, 0);
            if (!regex_error)
            {
                char pid[BUFFER], cmdline[BUFFER];
                strcpy(pid, entry->d_name);
                strcpy(procbuf, "/proc/");

                strcat(procbuf, entry->d_name);
                strcat(procbuf, "/cmdline");

                file = fopen(procbuf, "r");
                fgets(cmdline, BUFFER, file);

                printf(COLUMN_FORMAT, pid, cmdline);
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

void cd(char *args[], int args_count)
{

    if (args_count >= 2)
    {
        fprintf(stderr, "Wrong format, use cd <folder>\n");
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
        fprintf(stderr, "Fatal error %s\n", strerror(errno));
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
    printf("######     #    #     # ### ######     #    # ####### ######  ####### ####### ######     #    \n");
    printf("#     #   # #   #  #  #  #  #     #    #   #  #     # #     #      #  #       #     #   # #   \n");
    printf("#     #  #   #  #  #  #  #  #     #    #  #   #     # #     #     #   #       #     #  #   #  \n");
    printf("#     # #     # #  #  #  #  #     #    ###    #     # ######     #    #####   ######  #     # \n");
    printf("#     # ####### #  #  #  #  #     #    #  #   #     # #   #     #     #       #       ####### \n");
    printf("#     # #     # #  #  #  #  #     #    #   #  #     # #    #   #      #       #       #     # \n");
    printf("######  #     #  ## ##  ### ######     #    # ####### #     # ####### ####### #       #     # \n\n");
    printf("Developed by Dawid Korzepa © 2021\n\n");
    printf(COLUMN_FORMAT, "clear", "There will be a cool info.");
    printf(COLUMN_FORMAT, "help", "There will be a cool info.");
    printf(COLUMN_FORMAT, "exit", "There will be a cool info.");
    printf(COLUMN_FORMAT, "cd", "There will be a cool info.");
    printf(COLUMN_FORMAT, "ps", "There will be a cool info.");
    printf("\n\n");
}