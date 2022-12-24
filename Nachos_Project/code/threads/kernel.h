// kernel.h
//	Global variables for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef KERNEL_H
#define KERNEL_H

#include "copyright.h"
#include "debug.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "alarm.h"
#include "filesys.h"
#include "machine.h"
#include<map>

class SynchConsoleInput;
class SynchConsoleOutput;
class Ethernet_Layer;
class IP_Layer;
class UDP_Layer;
class SynchDisk;
class Datagram_Struct;
class Semaphore;
#include "bitmap.h"
#include "stable.h"
#include "ptable.h"

struct sock_struct
{
	unsigned char ip[4];
	int srcPort;
	int destPort;
};

class Kernel {
   public:
    Kernel(int argc, char **argv);
    // Interpret command line arguments
    ~Kernel();  // deallocate the kernel

    void Initialize(
        char *userProgName = NULL);  // initialize the kernel -- separated
                                     // from constructor because
                                     // refers to "kernel" as a global

    void ThreadSelfTest();  // self test of threads and synchronization

    void ConsoleTest();  // interactive console self test

    void NetworkTest();  // interactive 2-machine network test

    // These are public for notational convenience; really,
    // they're global variables used everywhere.

    Thread *currentThread;  // the thread holding the CPU
    Scheduler *scheduler;   // the ready list
    Interrupt *interrupt;   // interrupt status
    Statistics *stats;      // performance metrics
    Alarm *alarm;           // the software alarm clock
    Machine *machine;       // the simulated CPU
    SynchConsoleInput *synchConsoleIn;
    SynchConsoleOutput *synchConsoleOut;
    SynchDisk *synchDisk;
    FileSystem *fileSystem;

	Ethernet_Layer *ethernet_layer;
	IP_Layer *ip_layer;
	UDP_Layer *udp_layer;
    Semaphore *addrLock;
    Bitmap *gPhysPageBitMap;
    STable *semTab;
    PTable *pTab;

    int hostName;  // machine identifier

	int get_socket();
	int insert_connect_socket(int sockID, string ip, int srcPort, int destPort);
	int send_message(int sockID, char* msg);
	int insert_bind_socket(int sockID, int port);
	int getData(char* msg, int size, int sockID);
	void putData(char* data, int size, int port);
	void deleteSocket(int sockID);

   private:
    bool randomSlice;    // enable pseudo-random time slicing
    bool debugUserProg;  // single step user program
    double reliability;  // likelihood messages are dropped
    char *consoleIn;     // file to read console input from
    char *consoleOut;    // file to send console output to
	int current_socket;
	int get_ip_strToChar(string str, unsigned char* ip);
	map<int,struct sock_struct> connected_sockets;
	map<int,int> bind_sockets;
	map<int, Datagram_Struct> datagrams;

#ifndef FILESYS_STUB
    bool formatFlag;  // format the disk if this is true
	#endif
};

#endif  // KERNEL_H
