default: loop.c parse_wav.c
	gcc main.c fsm.c parse_wav.c autoloop.c loop.c -o main

ansi: loop.c parse_wav.c
	gcc main.c fsm.c parse_wav.c autoloop.c loop.c -o main -ansi -pedantic -Wall -Werror
