CC = gcc
CFLAGS = -Wall -Wextra -O2

main: main.c avl.c avl.h rb.c rb.h
	$(CC) $(CFLAGS) main.c avl.c rb.c -o main

run: main
	./main

validar: main
	./main validar

clean:
	rm -f main

.PHONY: run validar clean
