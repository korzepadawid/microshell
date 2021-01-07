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
#define GREP "grep"
#define MOVE "mv"
#define COPY "cp"
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

/**
 * Helpers
*/

void execute(char *args[]);
const char *path();
const char *user();
char *substring(char *string, int position, int length);
void parse_args(char *args[], char command[], int *args_count);
int index_of(char *a, char *b, int start);
char *lowercase(char *str);

/**
 * Shell programs
*/

void help();
void clear();
void history();
void move(char *args[], int args_count);
void grep(char *args[], int args_count);
void copy(char *args[], int args_count);
void change_dir(char *args[], int args_count);

/**
 * tree
*/

void print_nodes(char *node, int j);
void tree(char *args[], int args_count);

/**
 * copy
*/

bool exists(char *filename);
bool is_dir(char *path);
mode_t permissions_of(char *path);
void copy_structure(char *source, char *destination);
void copy_file(char *from, char *to);
void copy_directory(char *from, char *to);

int main()
{
    char *input;
    clear();
    rl_bind_key('\t', rl_complete);
    while (true)
    {
        char *args[BUFFER];
        int args_count = 0;
        printf("%s[%s%s%s:%s%s%s]%s\n", GREY, RED, user(), MAG, GRN, path(), GREY, RESET);
        input = readline("$ ");

        if (strlen(input) != 0)
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
        else if (strcmp(args[0], HISTORY) == 0)
        {
            history();
        }
        else if (strcmp(args[0], COPY) == 0)
        {
            copy(args, args_count);
        }
        else if (strcmp(args[0], GREP) == 0)
        {
            grep(args, args_count);
        }
        else if (strcmp(args[0], TREE) == 0)
        {
            tree(args, args_count);
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
            execute(args);
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

int index_of(char *a, char *b, int start)
{
    char temp[strlen(a)];
    strncpy(temp, a + start, strlen(a) - start);
    char *p = strstr(temp, b);
    return p ? p - temp + start : -1;
}

char *lowercase(char *str)
{
    unsigned char *temp_str = (unsigned char *)str;

    while (*temp_str)
    {
        *temp_str = tolower(*temp_str);
        temp_str++;
    }

    return str;
}

/**
 * Shell programs
*/

void grep(char *args[], int args_count)
{
    FILE *file;
    char pattern[BUFFER], source[BUFFER], line[BUFFER];
    bool ignore_case = false;

    if (args_count == 3)
    {
        strcpy(pattern, args[1]);
        strcpy(source, args[2]);
    }
    else if (args_count == 4 && (strcmp(args[1], "-i")) == 0)
    {
        ignore_case = true;
        strcpy(pattern, args[2]);
        strcpy(source, args[3]);
    }
    else
    {
        fprintf(stderr, RED "Wrong format, use grep <pattern> <source> or optional grep -i <pattern> <source> \n" RESET);
        return;
    }

    if ((file = fopen(source, "r")) == NULL)
    {
        fprintf(stderr, RED "Cannot open source\n" RESET);
        return;
    }

    while (fgets(line, BUFFER, file))
    {
        char dup_line[strlen(line)];
        strcpy(dup_line, line);
        char lowercase_line[strlen(line)];
        strcpy(lowercase_line, lowercase(dup_line));

        char dup_pattern[strlen(pattern)];
        strcpy(dup_pattern, pattern);
        char lowercase_pattern[strlen(pattern)];
        strcpy(lowercase_pattern, lowercase(dup_pattern));

        if ((strstr(line, pattern) && !ignore_case) || (strstr(lowercase_line, lowercase_pattern) && ignore_case))
        {
            int i;
            int pos = !ignore_case ? index_of(line, pattern, 0) : index_of(lowercase_line, lowercase_pattern, 0);
            for (i = 0; i < strlen(line); i++)
            {
                if (i == pos)
                {
                    printf(GRN);
                }
                else if (pos + strlen(pattern) == i)
                {
                    printf(RESET);
                    pos = !ignore_case ? index_of(line, pattern, i + 1) : index_of(lowercase_line, lowercase_pattern, i + 1);
                }
                putchar(line[i]);
            }
            printf(RESET);
        }
    }
    fclose(file);
}

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
    else if (strcmp(args[1], "-") == 0)
    {
        chdir(getenv("OLDPWD"));
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

void execute(char *args[])
{
    int pid;
    if ((pid = fork()) == 0)
    {
        execvp(args[0], args);
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
    printf(HELP_FORMAT, "grep", "There will be a cool info.");
    printf(HELP_FORMAT, "tree", "There will be a cool info.");
    printf(HELP_FORMAT, "cd", "There will be a cool info.");
    printf(HELP_FORMAT, "mv", "There will be a cool info.");
    printf(HELP_FORMAT, "history", "There will be a cool info.");
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
            if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) && entry->d_name[0] != '.')
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

void tree(char *args[], int args_count)
{
    char target[BUFFER];
    if (args_count > 2)
    {
        fprintf(stderr, RED "Wrong format, tree or tree <target> \n" RESET);
        return;
    }

    if (args_count == 1)
    {
        strcpy(target, path());
    }
    else
    {
        strcpy(target, args[1]);
    }

    print_nodes(target, 0);
}

/**
 * copy
*/

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
        return;
    }

    return stat_buffer.st_mode;
}

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

void copy(char *args[], int args_count)
{
    char from[BUFFER], to[BUFFER];
    if (args_count != 3)
    {
        fprintf(stderr, RED "Wrong format, use cp <source> <destination> \n" RESET);
        return;
    }

    strcpy(from, args[1]);
    strcpy(to, args[2]);

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