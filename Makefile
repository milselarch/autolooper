default: loop.c parse_wav.c
	gcc loop.c parse_wav.c -o loop

ansi: loop.c parse_wav.c
	gcc loop.c parse_wav.c -o loop -ansi -pedantic -Wall -Werror
