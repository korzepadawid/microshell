# microshell

A shell for UNIX-based operating systems.

## How to run?
```
$ sudo apt-get install libreadline-dev
```

```
$ make
```

```
$ ./microshell
```

## Description

- Developed support for double-quoted params, command history, TAB autocompletion, and up/down arrow keys to switch between previously executed commands.
- Handled SIGINT and SIGSTP signals.
- Implemented Krauss wildcard-matching algorithm, and used it in my own (recursive) implementation of the “find” command.
- Created my own (recursive) version of the "cp" command.

## Help
Have you got lost? Use `help`.
```
Features:
        double-quoted params
        colors
        user name in prompt
        history
        tab autocompletion
        up/down arrow key to switch between previously executed commands
        ^C support 
        ^Z support
Supported commands:
        clear:
                clear
                clears the terminal screen
        cd:
                cd [directory]
                changes the current working directory
        exit:
                exit
                causes the shell to exit
        help:
                help
                displays informations about shell features and author
        history:
                history
                displays previously executed commands
        cp:
                cp [source] [destination]
                recursively copies directories and files, with their permissions
        find:
                find [directory] [-name] [pattern] [-type] [d | f]
                searches a folder hierarchy for files that meet desired criteria, pattern supports Krauss wildcard-matching algorithm (?*)
```