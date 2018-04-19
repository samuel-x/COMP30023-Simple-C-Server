# 	Computer Systems COMP30023 Part A
#	name: Samuel Xu 
#   studentno: #835273
#	email: samuelx@student.unimelb.edu.au
#	login: samuelx
#
# 	Simple makefile for server.c

all: server.c
	gcc -g -Wall -o server server.c -lpthread

clean: 
	$(RM) server