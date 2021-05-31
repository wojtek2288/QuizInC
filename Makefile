CC = gcc
CFLAGS = -std=gnu99 -Wall -o

all: etap1 etap2 test server client client_s

etap1: etap1.c 
	${CC} ${CFLAGS} etap1 etap1.c

etap2: etap2.c 
	${CC} ${CFLAGS} etap2 etap2.c

test: test.c 
	${CC} ${CFLAGS} test test.c

server: server.c 
	${CC} ${CFLAGS} server server.c

client: client.c 
	${CC} ${CFLAGS} client client.c

client_s: client_s.c
	${CC} ${CFLAGS} client_s client_s.c
.PHONY: clean all

clean:
	rm etap1 etap2 test server client client_s