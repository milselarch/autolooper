default: loop.c parse_wav.c
	gcc loop.c parse_wav.c -o loop

ansi: loop.c parse_wav.c
	gcc loop.c parse_wav.c -o loop -ansi -pedantic -Wall -Werror

alt: alt-loop.c
	gcc alt-loop.c -lsndfile -o alt-loop
