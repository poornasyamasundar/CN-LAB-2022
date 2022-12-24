##User Programs
The Final Part of the Implementation is testing the system calls, and with it the entire network stack by writing two application programs **send.c** and **receive.c**. Then, we run *send.c* in machine 0 and *receive.c* in machine 1.

####send.c
In this program, we will create a socket, connect to machine 1, send a message and close the socket.

####receive.c
In this program, we will create a socket, bind it to a port, receive message from machine 0, print it and close the socket.

Now, to run these programs, we must first build them. Edit the [Makefile](Makefile) in test folder to add send and receive, just look how other files are added and simply follow them for send.c and receive.c

Now, run `'bash build_test.sh` in the root folder to compile the files.

Finally to test the User programs open three terminals. Run switch in one of them, and the two user programs in the remaining two terminals. To run a user program use *-x* flag and provide the path to the executable.

To run send.c and receive.c in build.linux folder run the below commands
`./nachos -m 0  -x ../test/send`, `./nachos -m 1 -x ../test/receive`.
