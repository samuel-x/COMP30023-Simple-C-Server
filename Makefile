# Simple makefile for server.c
all: server.c
	gcc -g -Wall -o server server.c

clean: 
	$(RM) server