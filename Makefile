all:
	gcc -o tri main.c `pkg-config --cflags --libs opencv`
