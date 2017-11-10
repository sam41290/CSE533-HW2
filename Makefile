# This is a sample Makefile which compiles source files named:
# - tcpechotimeserv.c
# - clientv.c
# - timecli.c
# - echocli.c
# and creating executables: "server", "client", "timecli"
# and "echocli", respectively.
#
# It uses various standard libraries, and the copy of Stevens'
# library "libunp.a" in ~cse533/Stevens/unpv13e_solaris2.10 .
#
# It also picks up the thread-safe version of "readline.c"
# from Stevens' directory "threads" and uses it when building
# the executable "server".
#
# It is set up, for illustrative purposes, to enable you to use
# the Stevens code in the ~cse533/Stevens/unpv13e_solaris2.10/lib
# subdirectory (where, for example, the file "unp.h" is located)
# without your needing to maintain your own, local copies of that
# code, and without your needing to include such code in the
# submissions of your assignments.
#
# Modify it as needed, and include it with your submission.

CC = gcc

LIBS = -lresolv -lsocket -lnsl -lpthread -lm\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a\
	
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: fclient fserver


# server uses the thread-safe version of readline.c




fserver: server.o readline.o rtt.o
	${CC} ${FLAGS} -o fserver server.o rtt.o readline.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c
rtt.o: rtt.c
	${CC} ${CFLAGS} -c rtt.c


fclient: client.o rtt.o
	${CC} ${FLAGS} -o fclient client.o rtt.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c


# pick up the thread-safe version of readline.c from directory "threads"

readline.o: /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c
	${CC} ${CFLAGS} -c /home/courses/cse533/Stevens/unpv13e_solaris2.10/threads/readline.c


clean:
	rm fserver server.o fclient client.o rtt.o readline.o

