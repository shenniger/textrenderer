all: yatref.o test

yatref.o: yatref.c
	$(CC) -Wall -Wextra -pedantic -std=c99 yatref.c -c -o yatref.o

test: yatref.o test.c
	$(CC) -Wall -Wextra -pedantic -std=c99 yatref.o test.c -o test -lSDL2 -lSDL2_ttf
