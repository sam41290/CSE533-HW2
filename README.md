# CSE533-HW2

All code in directory HW2

Files:
1. server.c
2. client.c
3. unprtt.h
4. rtt.c
5. mytypes.h
6. asgn2/client.in
7. asgn2/server.in
8. Makefile
9. testfiles/*

Running make generates two executables: fclient and fserver

Client supports following commands:

1. list: to list all files in server's working directory
2. get [filename]: to print a file
3. get [filename] [> or >>] [filename] : to download a file
4. quit: to exit client

testfiles/ directory a ASCII text file "smile". which contains an ASCII art of a smiling face.

Running "get smile" on client prints the art on the screen with proper order. This indicates that the client recieves the UDP datagrams in proper sequence.