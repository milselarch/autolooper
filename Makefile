default: test.c
	gcc test.c -lsndfile -o test

ansi: test.c
	gcc test.c -lsndfile -o test -ansi -pedantic -Wall -Werror
