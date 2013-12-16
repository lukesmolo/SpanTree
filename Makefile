CC=gcc
CFLAGS=-I include -Wall -pedantic -lpthread -ggdb -D_REENTRANT
OBJ = src/main.o src/threads.o src/net.o src/utils.o

%.o: %.c
		$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
		gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f ./src/*.o ./main

