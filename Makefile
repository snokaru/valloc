all:
	gcc -Wall -Werror -Iinclude/ -o alloc src/alloc.c src/main.c
