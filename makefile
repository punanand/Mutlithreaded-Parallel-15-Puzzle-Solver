SHELL=/bin/bash

CC=gcc

program:
	$(CC) -g puzzle_a.c -o puzzle_a.out -lpthread
	$(CC) -g puzzle_b.c -o puzzle_b.out -lpthread

clean:
	rm -rf *.out