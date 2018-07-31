myshell : shell.h shell.c test.c
	sudo gcc shell.h shell.c test.c -o myshell -lreadline -lcrypt
	sudo chmod u+s ./myshell
	