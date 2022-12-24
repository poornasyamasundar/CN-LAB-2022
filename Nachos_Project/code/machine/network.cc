// network.cc
//	Routines to simulate a network interface, using UNIX sockets
//	to deliver packets between multiple invocations of nachos.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "network.h"
#include "main.h"

//-----------------------------------------------------------------------
// NetworkInput::NetworkInput
// 	Initialize the simulation for the network input
//
//   	"toCall" is the interrupt handler to call when packet arrives
//-----------------------------------------------------------------------

NetworkInput::NetworkInput(CallBackObj *toCall) {
    // set up the stuff to emulate asynchronous interrupts
    callWhenAvail = toCall;
    packetAvail = FALSE;

    sock = OpenSocket();
	sprintf(sockName, "/tmp/machine%d", kernel->hostName);
    AssignNameToSocket(sockName, sock);  // Bind socket to a filename
                                         // in the current directory.

    // start polling for incoming packets
    kernel->interrupt->Schedule(this, NetworkTime, NetworkRecvInt);
}

//-----------------------------------------------------------------------
// NetworkInput::NetworkInput
// 	Deallocate the simulation for the network input
//		(basically, deallocate the input mailbox)
//-----------------------------------------------------------------------
NetworkInput::~NetworkInput() {
    CloseSocket(sock);
    DeAssignNameToSocket(sockName);
}

//-----------------------------------------------------------------------
// NetworkInput::CallBack
//	Simulator calls this when a packet may be available to //	be read in from the simulated network.
//
//      First check to make sure packet is available & there's space to
//	pull it in.  Then invoke the "callBack" registered by whoever
//	wants the packet.
//-----------------------------------------------------------------------

void NetworkInput::CallBack() {
    // schedule the next time to poll for a packet
    kernel->interrupt->Schedule(this, NetworkTime, NetworkRecvInt);

    if (!PollSocket(sock))  // do nothing if no packet to be read
        return;

    // otherwise, read packet in
    char *buffer = new char[1526];
    ReadFromSocket(sock, buffer, 1526);

	memcpy(current_packet, buffer, 1526);

    kernel->stats->numPacketsRecvd++;

    // tell post office that the packet has arrived
    callWhenAvail->CallBack();
}

//-----------------------------------------------------------------------
// NetworkInput::Receive
// 	Read a packet, if one is buffered
//-----------------------------------------------------------------------

void NetworkInput::Receive(char *data) 
{
	memcpy(data, current_packet, 1526);
}

NetworkOutput::NetworkOutput(double reliability, CallBackObj *toCall) {
    if (reliability < 0)
        chanceToWork = 0;
    else if (reliability > 1)
        chanceToWork = 1;
    else
        chanceToWork = reliability;

    callWhenDone = toCall;
    sendBusy = FALSE;
    sock = OpenSocket();
}

NetworkOutput::~NetworkOutput() { CloseSocket(sock); }

void NetworkOutput::CallBack() {
    sendBusy = FALSE;
    kernel->stats->numPacketsSent++;
}

void NetworkOutput::Send(unsigned char *buffer) {
	char toName[32];
	sprintf(toName, "/tmp/switch%d", kernel->hostName);

	kernel->interrupt->Schedule(this,NetworkTime,NetworkSendInt);
	
	SendToSocket(sock,(char*)buffer,1526,toName);
}
