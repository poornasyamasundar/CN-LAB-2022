#ifndef POST_H
#define POST_H

#include "copyright.h"
#include "utility.h"
#include "callback.h"
#include "network.h"
#include "synchlist.h" #include "synch.h"

struct Ethernet_Packet
{ unsigned char preamble[7];
	unsigned char SFD;
	unsigned char destMAC[6];	//48 bit destination mac address
	unsigned char srcMAC[6];	//48 bit source mac address
	unsigned short int ethertype;
	unsigned int crc;
	unsigned char payload[1500];	// payload of the ethernet packet.
}__attribute__((packed));

static unsigned char macpool[10][6] = 
    {{0xe5, 0xae, 0x32, 0x18, 0xd9, 0x21},
    {0xd5, 0xae, 0x32, 0x18, 0xd9, 0x21},
    {0xe5, 0x3e, 0x32, 0x18, 0xd9, 0x21},
    {0xe5, 0xae, 0x34, 0x18, 0xd9, 0x21},
    {0xe5, 0xa9, 0x32, 0x18, 0xd9, 0x21},
    {0xe5, 0xae, 0x32, 0x58, 0xd9, 0x21},
    {0xe5, 0xae, 0x32, 0x18, 0xa9, 0x21},
    {0xe5, 0xae, 0x32, 0x28, 0xd9, 0x21},
    {0xe5, 0xae, 0x66, 0x18, 0xd9, 0x21},
    {0xe5, 0xae, 0x32, 0x99, 0xd9, 0x21},
};

class Ethernet_Layer : public CallBackObj
{
	public:
		Ethernet_Layer();
		~Ethernet_Layer();

		void Receive(unsigned char* data);
		void Send(unsigned char* data, unsigned char* destMAC);

		static void pktrecv(void* data);
		static void ethrecv(void* data);

		void CallBack();
	
	private:
		NetworkOutput *networkOut;
		NetworkInput *networkIn;
		Lock *sendLock;
		Semaphore *messageAvailable;
		SynchList<unsigned char*>* messages;
};


#endif
