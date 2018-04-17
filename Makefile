# Simple makefile for server.c
all: server.c
	gcc -g -Wall -o server server.c -lpthread

clean: 
	$(RM) server