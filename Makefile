CC:=gcc
CFLAGS=-O3 -Wall
LIBS=-pthread
SRC=src

all: OS_Project1_Chatroom
OS_Project1_Chatroom: OS_Project1_Chatroom.o string.o
	$(CC) $(CFLAGS) $(LIBS) -o OS_Project1_Chatroom OS_Project1_Chatroom.o string.o
OS_Project1_Chatroom.o: OS_Project1_Chatroom.c
	$(CC) $(CFLAGS) -c OS_Project1_Chatroom.c
string.o: string.c
	$(CC) $(CFLAGS) -c string.c

.PHONY: clean
clean:
	rm -f *.o OS_Project1_Chatroom
