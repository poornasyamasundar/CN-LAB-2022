// kernel.cc
//	Initialization and cleanup routines for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "kernel.h"
#include "sysdep.h"
#include "synch.h"
#include "synchlist.h"
#include "libtest.h"
#include "string.h"
#include "synchconsole.h"
#include "synchdisk.h"
#include "post.h"

#define MAX_PROCESS 10
//----------------------------------------------------------------------
// Kernel::Kernel
// 	Interpret command line arguments in order to determine flags
//	for the initialization (see also comments in main.cc)
//----------------------------------------------------------------------

Kernel::Kernel(int argc, char **argv) {
    randomSlice = FALSE;
    debugUserProg = FALSE;
    consoleIn = NULL;   // default is stdin
    consoleOut = NULL;  // default is stdout
#ifndef FILESYS_STUB
    formatFlag = FALSE;
#endif
    reliability = 1;  // network reliability, default is 1.0
    hostName = 0;     // machine id, also UNIX socket name
                      // 0 is the default machine id
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-rs") == 0) {
            ASSERT(i + 1 < argc);
            RandomInit(atoi(argv[i + 1]));  // initialize pseudo-random
                                            // number generator
            randomSlice = TRUE;
            i++;
        } else if (strcmp(argv[i], "-s") == 0) {
            debugUserProg = TRUE;
        } else if (strcmp(argv[i], "-ci") == 0) {
            ASSERT(i + 1 < argc);
            consoleIn = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-co") == 0) {
            ASSERT(i + 1 < argc);
            consoleOut = argv[i + 1];
            i++;
#ifndef FILESYS_STUB
        } else if (strcmp(argv[i], "-f") == 0) {
            formatFlag = TRUE;
#endif
        } else if (strcmp(argv[i], "-n") == 0) {
            ASSERT(i + 1 < argc);  // next argument is float
            reliability = atof(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-m") == 0) {
            ASSERT(i + 1 < argc);  // next argument is int
            hostName = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-u") == 0) {
            cout << "Partial usage: nachos [-rs randomSeed]\n";
            cout << "Partial usage: nachos [-s]\n";
            cout << "Partial usage: nachos [-ci consoleIn] [-co consoleOut]\n";
#ifndef FILESYS_STUB
            cout << "Partial usage: nachos [-nf]\n";
#endif
            cout << "Partial usage: nachos [-n #] [-m #]\n";
        }
    }
}

//----------------------------------------------------------------------
// Kernel::Initialize
// 	Initialize Nachos global data structures.  Separate from the
//	constructor because some of these refer to earlier initialized
//	data via the "kernel" global variable.
//----------------------------------------------------------------------

void Kernel::Initialize(char *userProgName /*=NULL*/) {
    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state.
    currentThread = new Thread(userProgName);
    currentThread->setStatus(RUNNING);

    stats = new Statistics();        // collect statistics
    interrupt = new Interrupt;       // start up interrupt handling
    scheduler = new Scheduler();     // initialize the ready queue
    alarm = new Alarm(randomSlice);  // start up time slicing
    machine = new Machine(debugUserProg);
    synchConsoleIn = new SynchConsoleInput(consoleIn);     // input from stdin
    synchConsoleOut = new SynchConsoleOutput(consoleOut);  // output to stdout
    synchDisk = new SynchDisk();                           //
#ifdef FILESYS_STUB
    fileSystem = new FileSystem();
#else
    fileSystem = new FileSystem(formatFlag);
#endif  // FILESYS_STUB

	ethernet_layer = new Ethernet_Layer();
	ip_layer = new IP_Layer();
	udp_layer = new UDP_Layer();
	current_socket = 1;
    addrLock = new Semaphore("addrLock", 1);
    gPhysPageBitMap = new Bitmap(128);
    semTab = new STable();
    pTab = new PTable(MAX_PROCESS);

    interrupt->Enable();
}

//----------------------------------------------------------------------
// Kernel::~Kernel
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------

Kernel::~Kernel() {
    delete stats;
    delete interrupt;
    delete scheduler;
    delete alarm;
    delete machine;
    delete synchConsoleIn;
    delete synchConsoleOut;
    delete synchDisk;
    delete fileSystem;
    delete pTab;
	delete ethernet_layer;
	delete ip_layer;
	delete udp_layer;
    delete gPhysPageBitMap;
    delete semTab;
    delete addrLock;

    Exit(0);
}

//----------------------------------------------------------------------
// Kernel::ThreadSelfTest
//      Test threads, semaphores, synchlists
//----------------------------------------------------------------------

void Kernel::ThreadSelfTest() {
    Semaphore *semaphore;
    SynchList<int> *synchList;

    LibSelfTest();  // test library routines

    currentThread->SelfTest();  // test thread switching

    // test semaphore operation
    semaphore = new Semaphore("test", 0);
    semaphore->SelfTest();
    delete semaphore;

    // test locks, condition variables
    // using synchronized lists
    synchList = new SynchList<int>;
    synchList->SelfTest(9);
    delete synchList;
}

//----------------------------------------------------------------------
// Kernel::ConsoleTest
//      Test the synchconsole
//----------------------------------------------------------------------

void Kernel::ConsoleTest() {
    char ch;

    cout << "Testing the console device.\n"
         << "Typed characters will be echoed, until ^D is typed.\n"
         << "Note newlines are needed to flush input through UNIX.\n";
    cout.flush();

    do {
        ch = synchConsoleIn->GetChar();
        if (ch != EOF) synchConsoleOut->PutChar(ch);  // echo it!
    } while (ch != EOF);

    cout << "\n";
}

//----------------------------------------------------------------------
// Kernel::NetworkTest
//      Test whether the post office is working. On machines #0 and #1, do:
//
//      1. send a message to the other machine at mail box #0
//      2. wait for the other machine's message to arrive (in our mailbox #0)
//      3. send an acknowledgment for the other machine's message
//      4. wait for an acknowledgement from the other machine to our
//          original message
//
//  This test works best if each Nachos machine has its own window
//----------------------------------------------------------------------

void Kernel::NetworkTest() {
	if( kernel->hostName == 0 )
	{
		char* data = (char*)calloc(3000, sizeof(char));
		for( int i = 0 ; i < 1480 ; i++ )
		{
			data[i] = 'a';
		}
		for( int i = 1480 ; i < 2960 ; i++ )
		{
			data[i] = 'b';
		}
		for( int i = 2960 ; i < 3000 ; i++ )
		{
			data[i] = 'c';
		}
		unsigned char destIP[4] = {0xae, 0x32, 0x18, 0xd9};
		udp_layer->Send(data, 3000, (char*)destIP, 0, 0);
	}
}

int Kernel::get_socket()
{
	return current_socket++;
}

//create a new connected socket pair
int Kernel::insert_connect_socket(int sockID, string ip, int srcPort, int destPort)
{
	struct sock_struct ss;
	ss.srcPort = srcPort;
	ss.destPort = destPort;
	if( get_ip_strToChar(ip, ss.ip) == -1 )
	{
		return -1;
	}
	connected_sockets[sockID] = ss;
	return 1;
}

int Kernel::send_message(int sockID, char* msg)
{
	if( connected_sockets.find(sockID) == connected_sockets.end() )
	{
		return 0;
	}

	kernel->udp_layer->Send(msg, strlen(msg), (char*)connected_sockets[sockID].ip, connected_sockets[sockID].srcPort, connected_sockets[sockID].destPort);
	return 1;
}

//convert the ip from string to 4-byte ip address
int Kernel::get_ip_strToChar(string str, unsigned char* ip)
{
	int i = 0;
	int j = 0;
	string intermediate = "";
	for( int i = 0 ; i < str.length() ; i++ )
	{
		if( str[i] == '.' )
		{
			if( j >= 4 )
			{
				return -1;
			}
			ip[j++] = stoi(intermediate);
			intermediate = "";
		}
		else
		{
			intermediate += str[i];
		}
	}
	if( j >= 4 )
	{
		return -1;
	}
	ip[j++] = stoi(intermediate);

	if( j != 4 )
	{
		return -1;
	}
	return 1;
}

//insert the socket-port pair into bind_socket map, also create an entry in datagrams map
int Kernel::insert_bind_socket(int sockID, int port)
{
	if( bind_sockets.find(port) != bind_sockets.end() )
	{
		return -1;
	}
	bind_sockets[port] = sockID;

	if( datagrams.find(port) == datagrams.end())
	{
		Datagram_Struct ds;
		ds.messages = new SynchList<pair<int,char*>>();
		datagrams[port] = ds;
	}

	return 1;
}

//get the datagram from the datagrams map
int Kernel::getData(char* msg, int size, int sockID)
{
	//if the socket is not binded then return -1
	int port = -1;
	for( auto i : bind_sockets )
	{
		if( i.second == sockID )
		{
			port = i.first;
			break;
		}
	}
	if( port == -1 )
	{
		return -1;
	}

	pair<int,char*> p = datagrams[port].messages->RemoveFront();

	memcpy(msg, p.second, min(size,p.first));

	return 1;
}

//put the datagram into the datagrams map
void Kernel::putData(char* data, int size, int port)
{
	if( datagrams.find(port) == datagrams.end())
	{
		Datagram_Struct ds;
		ds.messages = new SynchList<pair<int,char*>>();
		datagrams[port] = ds;
	}

	datagrams[port].messages->Append(make_pair(size,data));
	return;
}

void Kernel::deleteSocket(int sockID)
{
	//delete the socket from connected sockets.
	if( connected_sockets.find(sockID) != connected_sockets.end())
	{
		connected_sockets.erase(sockID);
	}

	//delete the socket from bind sockets
	int port = -1;
	for( auto i : bind_sockets )
	{
		if( i.second == sockID)
		{
			port = i.first;
			break;
		}
	}
	
	if( port != -1 )
	{
		bind_sockets.erase(port);
	}
}
