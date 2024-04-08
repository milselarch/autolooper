default: main.c fsm.c fsm.h parse_wav.c parse_wav.h autoloop.c autoloop.h loop.c loop.h
	gcc main.c fsm.c parse_wav.c autoloop.c loop.c -o main

ansi: main.c fsm.c fsm.h parse_wav.c parse_wav.h autoloop.c autoloop.h loop.c loop.h
	gcc main.c fsm.c parse_wav.c autoloop.c loop.c -o main -ansi -pedantic -Wall -Werror
