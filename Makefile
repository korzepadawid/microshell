microshell: microshell.c
	gcc -ansi -Wall -o microshell microshell.c -I/usr/include/readline -lreadline

clean:
	rm -f microshell
