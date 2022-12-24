#ifndef POST_H
#define POST_H

#include "copyright.h"
#include "utility.h"
#include "callback.h"
#include "network.h"
#include "synchlist.h" 
#include "synch.h"
#include<vector>
#include<unordered_map>

struct Ethernet_Packet
{ 
	unsigned char preamble[7];
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

struct IP_Fragment
{
	unsigned int version: 4;
	unsigned int IHL: 4;
	unsigned int DSCP: 6;
	unsigned int ECN: 2;
	unsigned short totalLength;
	unsigned short identification;
	unsigned int reserved: 1;
	unsigned int DF: 1;
	unsigned int MF: 1;
	unsigned int fragmentOffset: 13;
	unsigned int timeToLive: 8;
	unsigned int protocol: 8;
	unsigned short checksum;
	unsigned char srcIP[4];
	unsigned char destIP[4];
	char payload[1480];

}__attribute__((packed));

struct Fragment
{
	bool MF;
	char* data;
	int offset;
};

static unsigned char ippool[10][4] = {
	{0xe5, 0xae, 0x32, 0x18},
    {0xae, 0x32, 0x18, 0xd9},
    {0x3e, 0x32, 0x18, 0xd9},
    {0xae, 0x34, 0x18, 0xd9},
    {0xa9, 0x32, 0x18, 0xd9},
    {0xae, 0x32, 0x58, 0xd9},
    {0xae, 0x32, 0x18, 0xa9},
    {0xae, 0x32, 0x28, 0xd9},
    {0xae, 0x66, 0x18, 0xd9},
    {0xae, 0x32, 0x99, 0xd9},
};

class IP_Layer
{
	public:
		IP_Layer();

		void Receive(unsigned char* data);
		void Send(unsigned char* data, int size, unsigned char* destIP);

	private:
		unordered_map<string, vector<struct Fragment>> fragmentDict;
		string computeHash(unsigned char* srcIP, unsigned char* destIP, int identification);
		void check(string key);
		void insertToMap(string key, IP_Fragment* ip_fragment);
		short get_Random_Identification();
};

struct UDP_Datagram
{
	unsigned short int sourcePort;
	unsigned short int destinationPort;
	unsigned short int length;
	unsigned short int checksum;
	char data[65536];
}__attribute__((packed));

class UDP_Layer
{
	public:
		UDP_Layer();

		void Send(char* data, int size, char* destIP, int srcPort, int destPort);
		void Receive(unsigned char* data);
};

class Datagram_Struct
{
	public:
		SynchList<pair<int,char*>> *messages;
};
#endif
