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
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>

#define EXIT "exit"
#define HELP "help"
#define TREE "tree"
#define CLEAR "clear"
#define HISTORY "history"
#define COPY "cp"
#define FIND "find"
#define CHANGE_DIR "cd"

#define MAG "\e[0;35m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define HCYN "\e[0;96m"
#define GREY "\x1B[90m"
#define YEL "\e[0;33m"
#define REDB "\e[41m"
#define CYN "\e[0;36m"
#define RESET "\x1B[0m"

#define HELP_FORMAT "%-9s %s \n"
#define HISTORY_FORMAT "%-3d %s \n"

#define BUFFER 1024

#define IDLE 0
#define QUOTES 1
#define WORD 2

/**
 * Helpers
*/

void execute(char *argv[]);
const char *path();
const char *user();
const char *home_dir();
void parse_args(char *argv[], int *argc, char *cmd);
bool exists(char *filename);
bool is_dir(char *path);
mode_t permissions_of(char *path);

/**
 * Shell programs
*/

void help();
void clear();
void history();
void copy(char *argv[], int argc);
void change_dir(char *argv[], int argc);

/**
 * tree
*/

void print_nodes(char *node, int j);
void tree(char *argv[], int argc);

/**
 * copy
*/

void copy_structure(char *source, char *destination);
void copy_file(char *from, char *to);
void copy_directory(char *from, char *to);

/**
 * find
*/

bool match(char *str, char *pattern, int str_len, int pat_len);
void find_recursively(char *path, char *pattern, bool dir_search, bool file_search);
void find(char *argv[], int argc);

char prev_dir[BUFFER];

int main()
{
    clear();
    rl_bind_key('\t', rl_complete);
    strcat(prev_dir, getenv("OLDPWD"));
    while (true)
    {
        char *argv[BUFFER];
        char *cmd;
        int argc = 0;
        printf("%s[%s%s%s:%s%s%s]%s\n", GREY, RED, user(), MAG, GRN, path(), GREY, RESET);
        cmd = readline("$ ");

        if (strlen(cmd) != 0)
        {
            add_history(cmd);
        }

        parse_args(argv, &argc, cmd);

        if (argv[0] == NULL)
        {
            continue;
        }
        else if (strcmp(argv[0], EXIT) == 0)
        {
            exit(0);
        }
        else if (strcmp(argv[0], HELP) == 0)
        {
            help();
        }
        else if (strcmp(argv[0], CLEAR) == 0)
        {
            clear();
        }
        else if (strcmp(argv[0], HISTORY) == 0)
        {
            history();
        }
        else if (strcmp(argv[0], FIND) == 0)
        {
            find(argv, argc);
        }
        else if (strcmp(argv[0], COPY) == 0)
        {
            copy(argv, argc);
        }
        else if (strcmp(argv[0], TREE) == 0)
        {
            tree(argv, argc);
        }
        else if (strcmp(argv[0], CHANGE_DIR) == 0)
        {
            change_dir(argv, argc);
        }
        else
        {
            execute(argv);
        }
        free(cmd);
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

void parse_args(char *argv[], int *argc, char *cmd)
{
    int current_char, state = IDLE, i = 0;
    char *ptr, *begin;
    ptr = cmd;
    while (ptr != NULL && *ptr != '\0')
    {
        current_char = *ptr;

        if (state == IDLE)
        {
            if (current_char == '\"')
            {
                state = QUOTES;
                begin = ptr + 1;
            }
            else if (current_char != ' ')
            {
                state = WORD;
                begin = ptr;
            }
        }
        else if ((state == QUOTES && current_char == '\"') || (state == WORD && current_char == ' '))
        {
            *ptr = 0;
            argv[i++] = begin;
            state = IDLE;
        }

        ptr++;
    }

    if (state != IDLE)
    {
        argv[i++] = begin;
    }

    *argc = i;
    argv[i] = NULL;
}

bool exists(char *filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

bool is_dir(char *path)
{
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

mode_t permissions_of(char *path)
{
    struct stat stat_buffer;

    if (stat(path, &stat_buffer) == -1)
    {
        fprintf(stderr, RED "Cannot get permissions. \n" RESET);
        exit(EXIT_FAILURE);
    }

    return stat_buffer.st_mode;
}

/**
 * Shell programs
*/

void clear()
{
    printf("\e[1;1H\e[2J");
}

void change_dir(char *argv[], int argc)
{
    char dest[BUFFER], current[BUFFER], *home;

    strcpy(current, path());

    if (argc > 2)
    {
        fprintf(stderr, RED "Wrong format, use cd <directory>\n" RESET);
        return;
    }

    if (argc == 1 || strcmp(argv[1], "~") == 0)
    {
        if ((home = getenv("HOME")) == NULL)
        {
            home = getpwuid(getuid())->pw_dir;
        }
        strcpy(dest, home);
    }
    else if (strcmp(argv[1], "-") == 0)
    {
        strcpy(dest, prev_dir);
        printf("%s\n", dest);
    }
    else
    {
        strcpy(dest, argv[1]);
    }

    if (chdir(dest) != 0)
    {
        fprintf(stderr, RED "Fatal error: %s\n" RESET, strerror(errno));
        return;
    }
    else
    {
        strcpy(prev_dir, current);
    }
}

void execute(char *argv[])
{
    int pid;
    if ((pid = fork()) == 0)
    {
        execvp(argv[0], argv);
        fprintf(stderr, RED "Unknown command, type help if you got lost\n" RESET);
        exit(1);
    }
    else
    {
        wait(NULL);
    }
}

void history()
{
    register HIST_ENTRY **hist_array;
    int i;
    if ((hist_array = history_list()) == NULL)
    {
        fprintf(stderr, RED "Cannot get history\n" RESET);
        return;
    }
    for (i = 0; hist_array[i]; i++)
    {
        printf(HISTORY_FORMAT, i + history_base, hist_array[i]->line);
    }
}

void help()
{
    printf(HELP_FORMAT, "clear", "There will be a cool info.");
    printf(HELP_FORMAT, "help", "There will be a cool info.");
    printf(HELP_FORMAT, "exit", "There will be a cool info.");
    printf(HELP_FORMAT, "cd", "There will be a cool info.");
    printf(HELP_FORMAT, "find", "There will be a cool info.");
    printf(HELP_FORMAT, "history", "There will be a cool info.");
    printf(HELP_FORMAT, "tree", "There will be a cool info.");
    printf(HELP_FORMAT, "cp", "There will be a cool info.");
    printf(GRN "Developed by Dawid Korzepa © 2021\n" RESET);
    printf(GRN "UAM INFORMATYKA ST 2020-2024\n" RESET);
}

/**
 * tree
*/

void print_nodes(char *node, int j)
{
    struct dirent *entry;
    DIR *dir;

    if (((dir = opendir(node)) == NULL) && (j == 0))
    {
        fprintf(stderr, RED "Unknown path\n" RESET);
        return;
    }

    if (dir)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) && (entry->d_name[0] != '.'))
            {
                int i;
                for (i = 0; i < j; i++)
                {
                    if (i % 2 == 0)
                    {
                        printf("%s", "|");
                    }
                    else
                    {
                        printf("\t");
                    }
                }

                printf("%s%s%s%s\n", "|--", entry->d_type == 4 ? YEL : RESET, entry->d_name, RESET);

                char target[BUFFER];

                strcpy(target, node);
                strcat(target, "/");
                strcat(target, entry->d_name);

                print_nodes(target, j + 2);
            }
        }
    }

    closedir(dir);
}

void tree(char *argv[], int argc)
{
    char target[BUFFER];
    if (argc > 2)
    {
        fprintf(stderr, RED "Wrong format, tree or tree <target> \n" RESET);
        return;
    }

    if (argc == 1)
    {
        strcpy(target, path());
    }
    else
    {
        strcpy(target, argv[1]);
    }

    print_nodes(target, 0);
}

/**
 * copy
*/

void copy_directory(char *from, char *to)
{
    umask(0);

    int result = mkdir(to, permissions_of(from));
    if (result < 0)
    {
        rmdir(to);
        mkdir(to, permissions_of(from));
    }
}

void copy_file(char *from, char *to)
{
    umask(0);

    int fd_in, fd_out, bytes;
    char buffer[BUFFER];

    fd_in = open(from, O_RDONLY);
    fd_out = open(to, O_RDWR | O_CREAT | O_EXCL, permissions_of(from));

    if (fd_out < 0)
    {
        fd_out = creat(to, permissions_of(from));
    }

    while ((bytes = read(fd_in, &buffer, BUFFER)) > 0)
    {
        write(fd_out, &buffer, bytes);
    }

    close(fd_in);
    close(fd_out);
}

void copy_structure(char *source, char *destination)
{
    char temp_source[BUFFER], temp_destination[BUFFER];
    struct dirent *entry;
    DIR *dir;

    if ((dir = opendir(source)) != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
            {
                strcpy(temp_source, source);
                strcpy(temp_destination, destination);

                strcat(temp_source, "/");
                strcat(temp_destination, "/");

                strcat(temp_source, entry->d_name);
                strcat(temp_destination, entry->d_name);

                if (is_dir(temp_source))
                {
                    copy_directory(temp_source, temp_destination);
                }
                else
                {
                    copy_file(temp_source, temp_destination);
                }

                copy_structure(temp_source, temp_destination);
            }
        }
    }
    closedir(dir);
}

void copy(char *argv[], int argc)
{
    char from[BUFFER], to[BUFFER];
    if (argc != 3)
    {
        fprintf(stderr, RED "Wrong format, use cp <source> <destination> \n" RESET);
        return;
    }

    strcpy(from, argv[1]);
    strcpy(to, argv[2]);

    if (!exists(from))
    {
        fprintf(stderr, RED "Unknown path \n" RESET);
        return;
    }

    umask(0);

    if (is_dir(from) && is_dir(to) && exists(to))
    {
        char path[BUFFER];
        strcpy(path, to);
        strcat(path, "/");
        strcat(path, basename(from));
        mkdir(path, permissions_of(from));
        copy_structure(from, path);
    }
    else if (is_dir(from) && !exists(to))
    {
        mkdir(to, permissions_of(from));
        copy_structure(from, to);
    }
    else if (!is_dir(from) && exists(from) && !exists(to))
    {
        copy_file(from, to);
    }
    else if (!is_dir(from) && exists(from) && exists(to) && is_dir(to))
    {
        char path[BUFFER];
        strcpy(path, to);
        copy_file(from, to);
        strcat(path, "/");
        strcat(path, basename(from));
        copy_file(from, path);
    }
    else
    {
        fprintf(stderr, RED "Unknown path \n" RESET);
        return;
    }
}

/**
 * find
*/

bool match(char *str, char *pattern, int str_len, int pat_len)
{
    if (pat_len == 0)
    {
        return (str_len == 0);
    }

    bool search[str_len + 1][pat_len + 1];
    int i, j;

    for (i = 0; i <= str_len; i++)
    {
        for (j = 0; j <= pat_len; j++)
        {
            search[i][j] = false;
        }
    }

    search[0][0] = true;
    for (j = 1; j <= pat_len; j++)
    {
        if (pattern[j - 1] == '*')
        {
            search[0][j] = search[0][j - 1];
        }
    }

    for (i = 1; i <= str_len; i++)
    {
        for (j = 1; j <= pat_len; j++)
        {
            if (pattern[j - 1] == '*')
            {
                search[i][j] = search[i][j - 1] || search[i - 1][j];
            }
            else if (pattern[j - 1] == '?' || str[i - 1] == pattern[j - 1])
            {
                search[i][j] = search[i - 1][j - 1];
            }
            else
            {
                search[i][j] = false;
            }
        }
    }

    return search[str_len][pat_len];
}

void find_recursively(char *path, char *pattern, bool dir_search, bool file_search)
{
    struct dirent *entry;
    DIR *dir;

    if ((dir = opendir(path)) != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
            {
                char new_target[BUFFER];
                strcpy(new_target, path);
                strcat(new_target, "/");
                strcat(new_target, entry->d_name);

                if ((match(entry->d_name, pattern, strlen(entry->d_name), strlen(pattern))))
                {
                    if (is_dir(new_target) && dir_search)
                    {
                        printf(YEL "%s\n" RESET, new_target);
                    }

                    if (!is_dir(new_target) && exists(new_target) && file_search)
                    {
                        printf("%s\n", new_target);
                    }
                }

                find_recursively(new_target, pattern, dir_search, file_search);
            }
        }
    }
    closedir(dir);
}

void find(char *argv[], int argc)
{
    char path[BUFFER], pattern[BUFFER];
    bool dirs, files, format_error;

    dirs = true;
    files = true;
    format_error = false;

    if (argc == 4 || argc == 6)
    {
        strcpy(path, argv[1]);
        if (strcmp(argv[2], "-name") == 0)
        {
            strcpy(pattern, argv[3]);
        }
        else
        {
            format_error = true;
        }
    }
    else
    {
        format_error = true;
    }

    if (argc == 6)
    {
        if ((strcmp(argv[4], "-type") == 0) && ((strcmp(argv[5], "d") == 0) || (strcmp(argv[5], "f") == 0)))
        {
            if ((strcmp(argv[5], "d") == 0))
            {
                dirs = true;
                files = false;
            }
            else
            {
                dirs = false;
                files = true;
            }
        }
        else
        {
            format_error = true;
        }
    }

    if (format_error)
    {
        fprintf(stderr, RED "Wrong format, use find <path> -name <name> -type [df] or find <path> -name <name>\n" RESET);
        return;
    }

    if (!(is_dir(path) && exists(path)))
    {
        fprintf(stderr, HCYN "Unknown path to directory \n" RESET);
        return;
    }

    find_recursively(path, pattern, dirs, files);
}