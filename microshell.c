#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <history.h>
#include <pwd.h>
#include <readline.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define EXIT "exit"
#define HELP "help"
#define CLEAR "clear"
#define CD "cd"
#define PS "ps"
#define LS "ls"

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define HCYN "\e[0;96m"
#define GREY "\x1B[90m"
#define RESET "\x1B[0m"

#define PS_FORMAT "%-9s %s \n"
#define HELP_FORMAT "%-9s %s \n"
#define LS_FORMAT "%-10s %-6s %-6s %-15s %-15s %-18s %s\n"

#define BUFFER 1024

/**
* Shell programs
*/

void help();
void clear();
void ps();
void cd(char *args[], int args_count);
void ls(char *args[], int args_count);
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
    char *input, shell_prompt[BUFFER];
    rl_bind_key('\t', rl_complete);
    while (true)
    {

        char *args[BUFFER];
        int args_count = 0;

        sprintf(shell_prompt, "%s[%s%s%s:%s%s%s]\n$ %s", GREY, RED, user(), GREY, HCYN, path(), GREY, RESET);
        input = readline(shell_prompt);

        parse_args(args, input, &args_count);

        if (args[0] != NULL)
        {
            add_history(input);
        }

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
        else if (strcmp(args[0], LS) == 0)
        {
            ls(args, args_count);
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

void ls(char *args[], int args_count)
{
    DIR *dir;
    struct dirent *file;
    struct stat file_meta;
    register struct passwd *pw;
    register struct group *gwd;

    if (args_count > 1)
    {
        fprintf(stderr, RED "Wrong format, use ls <path>.\n" RESET);
        return;
    }

    if ((dir = opendir(args[1] == NULL ? path() : args[1])) == NULL)
    {
        fprintf(stderr, RED "Unknown path.\n" RESET);
        return;
    }

    stat(args[1] == NULL ? path() : args[1], &file_meta);

    printf(GRN LS_FORMAT RESET, "access", "links", "size", "group", "user", "date", "filename");

    while ((file = readdir(dir)) != NULL)
    {
        stat(file->d_name, &file_meta);

        printf((S_ISDIR(file_meta.st_mode)) ? "d" : "-");

        printf((file_meta.st_mode & S_IRUSR) ? "r" : "-");
        printf((file_meta.st_mode & S_IWUSR) ? "w" : "-");
        printf((file_meta.st_mode & S_IXUSR) ? "x" : "-");
        printf((file_meta.st_mode & S_IRGRP) ? "r" : "-");
        printf((file_meta.st_mode & S_IWGRP) ? "w" : "-");
        printf((file_meta.st_mode & S_IXGRP) ? "x" : "-");
        printf((file_meta.st_mode & S_IROTH) ? "r" : "-");
        printf((file_meta.st_mode & S_IWOTH) ? "w" : "-");
        printf((file_meta.st_mode & S_IXOTH) ? "x" : "-");

        pw = getpwuid(file_meta.st_uid);
        gwd = getgrgid(file_meta.st_gid);

        printf(" ");
        printf("%-6ld ", file_meta.st_nlink);
        printf("%-6ld ", file_meta.st_size);
        printf("%-15s ", gwd->gr_name);
        printf("%-15s ", pw->pw_name);
        printf("%-18s ", substring(ctime(&file_meta.st_mtime), 5, 16));
        printf("%s\n", file->d_name);
    }

    closedir(dir);
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
        fprintf(stderr, RED "regex error.\n" RESET);
        exit(EXIT_FAILURE);
    }

    if ((dir = opendir("/proc/")) == NULL)
    {
        fprintf(stderr, RED "/proc/ error \n" RESET);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(GRN PS_FORMAT RESET, "PID", "CMD");
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

                printf(PS_FORMAT, pid, cmdline);
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
        fprintf(stderr, RED "Wrong format, use cd <folder>\n" RESET);
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