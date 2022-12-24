#include "copyright.h"
#include "post.h"
#include<iostream>
using namespace std;

Ethernet_Layer::Ethernet_Layer()
{
	sendLock = new Lock("message send lock");
	networkOut = new NetworkOutput(1, this);
	networkIn  = new NetworkInput(this);

	Thread *s = new Thread("pktrecv");
	s->Fork(Ethernet_Layer::pktrecv,this);

	Thread *t = new Thread("ethrecv");
	t->Fork(Ethernet_Layer::ethrecv,this);

	messageAvailable = new Semaphore("message Avaialable",0);

	messages = new SynchList<unsigned char*>;
}

Ethernet_Layer::~Ethernet_Layer()
{
	delete networkOut;
	delete networkIn;
	delete sendLock;
}

void Ethernet_Layer::Send(unsigned char* data, unsigned char* destMAC)
{
	Ethernet_Packet eth;
	eth.crc = 0;
	eth.ethertype = 0x0800;
	eth.SFD = 0;
	for( int i = 0 ; i < 7 ; i++ )
	{
		eth.preamble[i] = 0;
	}
	memcpy(eth.srcMAC,macpool[kernel->hostName],6);
	memcpy(eth.destMAC, destMAC,6);
	memcpy(eth.payload, data, 1500);

	unsigned char* buffer = (unsigned char*) calloc(sizeof(Ethernet_Packet), sizeof(unsigned char));
	memcpy(buffer, (const unsigned char*)&eth, sizeof(Ethernet_Packet));

	sendLock->Acquire();
	networkOut->Send(buffer);
	sendLock->Release();
}

void Ethernet_Layer::pktrecv(void* data)
{
	Ethernet_Layer *_this = (Ethernet_Layer*)data;

	//run an infinite loop
	for(;;)
	{
		//first wait for a message
		_this->messageAvailable->P();


		//get the packet;
		char* bf = (char*)calloc(1526, sizeof(char));
		_this->networkIn->Receive(bf);

		Ethernet_Packet *eth;
		eth = (Ethernet_Packet*)bf;

		//check if the packet actually came for us by comparing the destmac with our mac.
		bool b = true;
		for( int i = 0 ; i < 6 ; i++ )
		{
			if( eth->destMAC[i] != macpool[kernel->hostName][i] )
			{
				b = false;
				break;
			}
		}

		//if the packet belongs to us, then call Ethernet_Layer.Receive(), else simply drop the packet.
		if( b )
		{
			unsigned char* buffer = (unsigned char*) calloc(sizeof(Ethernet_Packet), sizeof(unsigned char));
			memcpy(buffer, (const unsigned char*)eth, sizeof(Ethernet_Packet));

			_this->messages->Append(buffer);
		}
	}
}

void Ethernet_Layer::ethrecv(void* data)
{
	Ethernet_Layer *_this = (Ethernet_Layer*)data;
	//continuously call Ethernet_Layer.Receive();
	for(;;)
	{
		unsigned char* data = (unsigned char*)calloc(1500, sizeof(char));
		_this->Receive(data);
		//kernel->ip_layer->Receive(data);
	}
}

void Ethernet_Layer::Receive(unsigned char* data)
{
	unsigned char* buffer = messages->RemoveFront(); 

	Ethernet_Packet* eth;
	eth = (Ethernet_Packet* )buffer;
	
	memcpy(data, eth->payload, 1500);

	cout << "received data is : " << eth->payload << endl;
}

void Ethernet_Layer::CallBack()
{
	messageAvailable->V();
}
