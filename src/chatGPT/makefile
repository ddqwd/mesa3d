.PHONY: clean, mrproper
CC = gcc
CFLAGS = -g -Wall
LINKS= -lGLEW -lGL -lglfw -lGLU
all: demo

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

demo: demo.o
	$(CC) $(CFLAGS) -o $@ $+ $(LINKS) 

clean:
	rm -f *.o core.*

mrproper: clean
	rm -f demo
