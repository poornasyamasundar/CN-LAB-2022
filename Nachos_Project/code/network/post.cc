#include "copyright.h"
#include "post.h"
#include<iostream>
#include<cmath>
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
		kernel->ip_layer->Receive(data);
	}
}

void Ethernet_Layer::Receive(unsigned char* data)
{
	unsigned char* buffer = messages->RemoveFront(); 

	Ethernet_Packet* eth;
	eth = (Ethernet_Packet* )buffer;
	
	memcpy(data, eth->payload, 1500);
}

void Ethernet_Layer::CallBack()
{
	messageAvailable->V();
}

IP_Layer::IP_Layer(){}

short IP_Layer::get_Random_Identification()
{
	return rand()%(64*1024);
}

string IP_Layer::computeHash(unsigned char* srcIP, unsigned char* destIP, int identification)
{
	string key = "";
	for( int i = 0 ; i < 4 ; i++ )
	{
		key += srcIP[i] + destIP[i];
	}

	key += to_string(identification);

	return key;
}

void IP_Layer::Send(unsigned char* data, int size, unsigned char* destIP)
{
	//Declare all the IP Fields
	const int IPHEADERSIZE = 20;
	const int ETHPAYLOADSIZE = 1500;
	const int UDPPROTOCOL = 17;
	const int VERSION = 4;
	const int IHL = 5;
	const int DSCP = 0;
	const int ECN = 0;
	const int TTL = 1;
	const int CHECKSUM = 0;
	const unsigned char* DESTIP = destIP;
	const unsigned char* SRCIP = ippool[kernel->hostName];

	int ip_payload_size = (ETHPAYLOADSIZE - IPHEADERSIZE);
	
	int num_of_fragments = ceil((double)size/(double)ip_payload_size);
	const int total_len = size + num_of_fragments*20;
	short identification = get_Random_Identification();

	//compute the destMAC from destIP
	int dest_machine = -1;
	for( int i = 0 ; i < 10 ; i++ )
	{
		bool b = true;
		for( int j = 0 ; j < 4 ; j++ )
		{
			if( destIP[j] != ippool[i][j] )
			{
				b = false;
				break;
			}
		}
		if( b )
		{
			dest_machine = i;
			break;
		}
	}

	if( dest_machine == -1 )
	{
		cout << "Unknown destination IP address" << endl;
	}

	for( int i = 0 ; i < num_of_fragments ; i++ )
	{
		IP_Fragment ip_fragment;
		ip_fragment.version = VERSION;
		ip_fragment.IHL = IHL;
		ip_fragment.DSCP = DSCP;
		ip_fragment.ECN = ECN;
		ip_fragment.totalLength = total_len;
		ip_fragment.identification = identification;
		ip_fragment.reserved = 0;
		ip_fragment.DF = 0;
		//this will be 1 for all packets except the last one
		ip_fragment.MF = (i < (num_of_fragments-1));
		ip_fragment.fragmentOffset = (i*ip_payload_size)/8;
		ip_fragment.timeToLive = TTL;
		ip_fragment.protocol = UDPPROTOCOL;
		ip_fragment.checksum = CHECKSUM;
		memcpy(ip_fragment.srcIP, SRCIP,4);
		memcpy(ip_fragment.destIP, DESTIP,4);
		memset(ip_fragment.payload, '\0', 1480*sizeof(char));
		memcpy(ip_fragment.payload, data+i*1480, min(size-i*1480, 1480));

		unsigned char* buffer = (unsigned char*)calloc(ETHPAYLOADSIZE, sizeof(char));
		memcpy(buffer, (const unsigned char*)&ip_fragment, sizeof(ip_fragment));

		kernel->ethernet_layer->Send(buffer, macpool[dest_machine]);
	}
}

void IP_Layer::insertToMap(string key, IP_Fragment* ip_fragment)
{
	//if this is the first fragment of a new datagram and insert a new (key, value) pair into the map
	if( fragmentDict.find(key) == fragmentDict.end() )
	{
		vector<struct Fragment> v;
		struct Fragment f;
		f.offset = ip_fragment->fragmentOffset;

		char* c = (char*) calloc(1480, sizeof(char));
		memcpy(c, ip_fragment->payload, 1480);

		f.data = c;
		f.MF = (ip_fragment->MF == 1);

		v.push_back(f);
		fragmentDict[key] = v;
	}
	//else, insert the new fragment in the appropriate position of the already existing (key, value) pair.
	else
	{
		struct Fragment f;
		f.offset = ip_fragment->fragmentOffset;

		char* c = (char*) calloc(1480, sizeof(char));
		memcpy(c, ip_fragment->payload, 1480);

		f.data = c;
		f.MF = (ip_fragment->MF == 1);

		int size = fragmentDict[key].size();
		for( int i = 0 ; i < size ; i++ )
		{
			if( fragmentDict[key][i].offset > ip_fragment->fragmentOffset )
			{
				fragmentDict[key].insert(fragmentDict[key].begin()+i, f);
				return;
			}
		}

		fragmentDict[key].push_back(f);
	}
}

void IP_Layer::check(string key)
{
	vector<struct Fragment> v = fragmentDict[key];
	for( int i = 0 ; i < v.size() ; i++ )
	{
		if( v[i].offset != i*185 )
		{
			return;
		}
	}

	if( v.back().MF == 0 )
	{
		int size = v.size();
		size *= 1480;
		char* datagram = (char*)calloc(size, sizeof(char));
		for( int i = 0 ; i < v.size() ; i++ )
		{
			memcpy(datagram+i*1480, v[i].data, 1480);
		}
		cout << "received datagram = " << datagram << endl;
		fragmentDict.erase(fragmentDict.find(key));
	}
}

void IP_Layer::Receive(unsigned char* data)
{
	IP_Fragment* ip_fragment;
	ip_fragment = (IP_Fragment*)data;

	string key = computeHash(ip_fragment->srcIP, ip_fragment->destIP, ip_fragment->identification);
	insertToMap(key, ip_fragment);
	check(key);
}
