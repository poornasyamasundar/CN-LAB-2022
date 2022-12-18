// post.cc
// 	Routines to deliver incoming network messages to the correct
//	"address" -- a mailbox, or a holding area for incoming messages.
//	This module operates just like the US postal service (in other
//	words, it works, but it's slow, and you can't really be sure if
//	your mail really got through!).
//
//	Note that once we prepend the MailHdr to the outgoing message data,
//	the combination (MailHdr plus data) looks like "data" to the Network
//	device.
//
// 	The implementation synchronizes incoming messages with threads
//	waiting for those messages.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "post.h"
#include<iostream>
using namespace std;

//----------------------------------------------------------------------
// Mail::Mail
//      Initialize a single mail message, by concatenating the headers to
//	the data.
//
//	"pktH" -- source, destination machine ID's
//	"mailH" -- source, destination mailbox ID's
//	"data" -- payload data
//----------------------------------------------------------------------

/*
Mail::Mail(PacketHeader pktH, MailHeader mailH, char *msgData) {
    ASSERT(mailH.length <= MaxMailSize);

    pktHdr = pktH;
    mailHdr = mailH;
    bcopy(msgData, data, mailHdr.length);
}
*/

Mail::Mail(Ethernet ethH)
{
	ethHdr = ethH;
}

//----------------------------------------------------------------------
// MailBox::MailBox
//      Initialize a single mail box within the post office, so that it
//	can receive incoming messages.
//
//	Just initialize a list of messages, representing the mailbox.
//----------------------------------------------------------------------

MailBox::MailBox() { messages = new SynchList<Mail *>(); }

//----------------------------------------------------------------------
// MailBox::~MailBox
//      De-allocate a single mail box within the post office.
//
//	Just delete the mailbox, and throw away all the queued messages
//	in the mailbox.
//----------------------------------------------------------------------

MailBox::~MailBox() { delete messages; }

//----------------------------------------------------------------------
// PrintHeader
// 	Print the message header -- the destination machine ID and mailbox
//	#, source machine ID and mailbox #, and message length.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//----------------------------------------------------------------------

static void PrintHeader(PacketHeader pktHdr, MailHeader mailHdr) {
    cout << "From (" << pktHdr.from << ", " << mailHdr.from << ") to ("
         << pktHdr.to << ", " << mailHdr.to << ") bytes " << mailHdr.length
         << "\n";
}

//----------------------------------------------------------------------
// MailBox::Put
// 	Add a message to the mailbox.  If anyone is waiting for message
//	arrival, wake them up!
//
//	We need to reconstruct the Mail message (by concatenating the headers
//	to the data), to simplify queueing the message on the SynchList.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//	"data" -- payload message data
//----------------------------------------------------------------------

/*
void MailBox::Put(PacketHeader pktHdr, MailHeader mailHdr, char *data) {
    Mail *mail = new Mail(pktHdr, mailHdr, data);

    messages->Append(mail);  // put on the end of the list of
                             // arrived messages, and wake up
                             // any waiters
}
*/
void MailBox::Put(Ethernet ethHdr)
{
	Mail* mail = new Mail(ethHdr);
	messages->Append(mail);
}

//----------------------------------------------------------------------
// MailBox::Get
// 	Get a message from a mailbox, parsing it into the packet header,
//	mailbox header, and data.
//
//	The calling thread waits if there are no messages in the mailbox.
//
//	"pktHdr" -- address to put: source, destination machine ID's
//	"mailHdr" -- address to put: source, destination mailbox ID's
//	"data" -- address to put: payload message data
//----------------------------------------------------------------------

/*
void MailBox::Get(PacketHeader *pktHdr, MailHeader *mailHdr, char *data) {
    DEBUG(dbgNet, "Waiting for mail in mailbox");
    Mail *mail = messages->RemoveFront();  // remove message from list;
                                           // will wait if list is empty

    *pktHdr = mail->pktHdr;
    *mailHdr = mail->mailHdr;
    if (debug->IsEnabled('n')) {
        cout << "Got mail from mailbox: ";
        PrintHeader(*pktHdr, *mailHdr);
    }
    bcopy(mail->data, data, mail->mailHdr.length);
    // copy the message data into
    // the caller's buffer
    delete mail;  // we've copied out the stuff we
                  // need, we can now discard the message
}*/

int MailBox::Get(Ethernet* eth)
{
	DEBUG(dbgNet, "Waiting for mail in mailbox");
    //Mail *mail = messages->RemoveFront();  // remove message from list;
                                           // will wait if list is empty

	Mail *mail = messages->RemoveFront();

    *eth = mail->ethHdr;
    delete mail;  // we've copied out the stuff we
	return 1;
}
//----------------------------------------------------------------------
// PostOfficeInput::PostOfficeInput
// 	Initialize the post office input queues as a collection of mailboxes.
//	Also initialize the network device, to allow post offices
//	on different machines to deliver messages to one another.
//
//      We use a separate thread "the postal worker" to wait for messages
//	to arrive, and deliver them to the correct mailbox.  Note that
//	delivering messages to the mailboxes can't be done directly
//	by the interrupt handlers, because it requires a Lock.
//
//	"nBoxes" is the number of mail boxes in this Post Office
//----------------------------------------------------------------------

PostOfficeInput::PostOfficeInput(int nBoxes) {
    messageAvailable = new Semaphore("message available", 0);

    numBoxes = nBoxes;
    boxes = new MailBox[nBoxes];

	iplayer = new IPLayer;

    network = new NetworkInput(this);

    Thread *t = new Thread("postal worker");

    t->Fork(PostOfficeInput::PostalDelivery, this);

	Thread *t1 = new Thread("postal Pickup");

	t1->Fork(PostOfficeInput::PostalPickup, this);
}

//----------------------------------------------------------------------
// PostOfficeInput::~PostOfficeInput
// 	De-allocate the post office data structures.
//
//	Since the postal helper is waiting on the "messageAvail" semaphore,
//	we don't deallocate it!  This leaves garbage lying about,
//	but the alternative is worse!
//----------------------------------------------------------------------

PostOfficeInput::~PostOfficeInput() {
    delete network;
    delete[] boxes;
}

void PostOfficeInput::PostalDelivery(void *data) 
{
    PostOfficeInput *_this = (PostOfficeInput *)data;
    
	for (;;) {
        // first, wait for a message
        _this->messageAvailable->P();
        Ethernet eth = _this->network->Receive();

		bool b = false;
		for( int i = 0 ; i < 6 ; i++ )
		{
			if( eth.destMAC[i] != macpool[kernel->hostName][i] )
			{
				b = true;
				break;
			}
		}
		if( !b )
		{
        	_this->boxes[0].Put(eth);
		}
    }
}

void PostOfficeInput::PostalPickup(void *data) 
{
    PostOfficeInput *_this = (PostOfficeInput *)data;
    
	for (;;) {
		
		char* data = (char*)calloc(1500, sizeof(char));
      	_this->Receive(data);

		_this->iplayer->Receive(data);
    }
}


//----------------------------------------------------------------------
// PostOfficeInput::Receive
// 	Retrieve a message from a specific box if one is available,
//	otherwise wait for a message to arrive in the box.
//
//	Note that the MailHeader + data looks just like normal payload
//	data to the Network.
//
//
//	"box" -- mailbox ID in which to look for message
//	"pktHdr" -- address to put: source, destination machine ID's
//	"mailHdr" -- address to put: source, destination mailbox ID's
//	"data" -- address to put: payload message data
//----------------------------------------------------------------------
/*
void PostOfficeInput::Receive(int box, PacketHeader *pktHdr,
                              MailHeader *mailHdr, char *data) {
    ASSERT((box >= 0) && (box < numBoxes));

    boxes[box].Get(pktHdr, mailHdr, data);
    ASSERT(mailHdr->length <= MaxMailSize);
}
*/
void PostOfficeInput::Receive(char* data)
{
	Ethernet eth;
	int status = boxes[0].Get(&eth);
	memcpy(data, eth.payload, 1500);
}

//----------------------------------------------------------------------
// PostOffice::CallBack
// 	Interrupt handler, called when a packet arrives from the network.
//
//	Signal the PostalDelivery routine that it is time to get to work!
//----------------------------------------------------------------------

void PostOfficeInput::CallBack() 
{ 
	messageAvailable->V(); 
}

//----------------------------------------------------------------------
// PostOfficeOutput::PostOfficeOutput
// 	Initialize the post office output queue.
//
//	"reliability" is the probability that a network packet will
//	  be delivered (e.g., reliability = 1 means the network never
//	  drops any packets; reliability = 0 means the network never
//	  delivers any packets)
//----------------------------------------------------------------------

PostOfficeOutput::PostOfficeOutput(double reliability) {
    messageSent = new Semaphore("message sent", 0);
    sendLock = new Lock("message send lock");

    network = new NetworkOutput(reliability, this);
}

//----------------------------------------------------------------------
// PostOfficeOutput::~PostOfficeOutput
// 	De-allocate the post office data structures.
//----------------------------------------------------------------------

PostOfficeOutput::~PostOfficeOutput() {
    delete network;
    delete messageSent;
    delete sendLock;
}

//----------------------------------------------------------------------
// PostOfficeOutput::Send
// 	Concatenate the MailHeader to the front of the data, and pass
//	the result to the Network for delivery to the destination machine.
//
//	Note that the MailHeader + data looks just like normal payload
//	data to the Network.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//	"data" -- payload message data
//----------------------------------------------------------------------

/*
void PostOfficeOutput::Send(PacketHeader pktHdr, MailHeader mailHdr,char *data) {
    char *buffer = new char[MaxPacketSize];  // space to hold concatenated
                                             // mailHdr + data

    if (debug->IsEnabled('n')) {
        cout << "Post send: ";
        PrintHeader(pktHdr, mailHdr);
    }
    ASSERT(mailHdr.length <= MaxMailSize);
    ASSERT(0 <= mailHdr.to);

    // fill in pktHdr, for the Network layer
    pktHdr.from = kernel->hostName;
    pktHdr.length = mailHdr.length + sizeof(MailHeader);

    // concatenate MailHeader and data
    bcopy((char *)&mailHdr, buffer, sizeof(MailHeader));
    bcopy(data, buffer + sizeof(MailHeader), mailHdr.length);

    sendLock->Acquire();  // only one message can be sent
                          // to the network at any one time
    network->Send(pktHdr, buffer);
    messageSent->P();  // wait for interrupt to tell us
                       // ok to send the next message
    sendLock->Release();

    delete[] buffer;  // we've sent the message, so
                      // we can delete our buffer
}
*/

void PostOfficeOutput::Send(unsigned char* data, int dest)
{
	Ethernet eth;
	memcpy(eth.srcMAC, macpool[kernel->hostName], 6);
	memcpy(eth.destMAC, macpool[dest], 6);
	memcpy(eth.payload, data, 1500);

	sendLock->Acquire();

	network->Send(eth);
	messageSent->P();

	sendLock->Release();
}

//----------------------------------------------------------------------
// PostOfficeOutput::CallBack
// 	Interrupt handler, called when the next packet can be put onto the
//	network.
//
//	Called even if the previous packet was dropped.
//----------------------------------------------------------------------

void PostOfficeOutput::CallBack() { messageSent->V(); }


short getRandomIndentificationNumber(){
    return rand()%(64*1024);
}
IPLayer::IPLayer(){
}
UDPLayer::UDPLayer(){
}
void IPLayer::Send(char* data,int size, int destMachine){
    // IP Header Fields
    const int IPHEADERSIZE = 20;
    const int ETHPAYLOADSIZE = 1500;
    const int UDPPROTOCOL = 17;
    const int VERSION = 4;
    const int IHL = 5;
    const int DSCP = 0;
    const int ECN = 0;
    const int TTL = 1;
    const int CHECKSUM = 0;
    const unsigned char* DESTIP = ippool[destMachine];
    const unsigned char* SRCIP = ippool[kernel->hostName];

    int space = (ETHPAYLOADSIZE - IPHEADERSIZE);
    
    int numOfPackets = (size+space-1)/(space);
    const int totalLength = size + numOfPackets*20;
    short identificationNumber = getRandomIndentificationNumber();
    
    for(int i = numOfPackets-1 ; i >= 0; i-- ){
        IPHeader iphdr;
        iphdr.version = VERSION;
        iphdr.IHL = IHL;
        iphdr.DSCP = DSCP;
        iphdr.ECN = ECN;
        iphdr.totalLength = totalLength;
        iphdr.identification = identificationNumber;
        iphdr.reserved = 0;
        iphdr.DF = 0;
        iphdr.MF = (i < (numOfPackets-1));
        // check this once
        iphdr.fragmentOffset = (i*space)/8;
        iphdr.timeToLive = TTL;
        iphdr.protocol = UDPPROTOCOL;
        iphdr.checksum = CHECKSUM;
        memcpy(iphdr.srcIP, SRCIP, 4);
        memcpy(iphdr.destIP, DESTIP, 4);
        memset(iphdr.payload,'\0', 1480*sizeof(char));
        memcpy(iphdr.payload, data+i*1480, min(size-i*1480, 1480));

        unsigned char* buffer = (unsigned char*)calloc(ETHPAYLOADSIZE, sizeof(char));
        memcpy(buffer, (const unsigned char*)&iphdr, sizeof(iphdr));
  
        kernel->postOfficeOut->Send(buffer, destMachine);
    }
}

void PrintPacket(IPHeader* iphdr)
{
	cout << "Received packet is " << endl;
	cout << "version = " << iphdr->version << endl;
	cout << "IHL = " << iphdr->IHL << endl;
	cout << "DSCP = " << iphdr->DSCP << endl;
	cout << "ECN = " << iphdr->ECN << endl;
	cout << "totalLength = " << iphdr->totalLength << endl;
	cout << "identification = " << iphdr->identification << endl;
	cout << "reserved = " << iphdr->reserved << endl;
	cout << "DF = " << iphdr->DF << endl;
	cout << "MF = " << iphdr->MF << endl;
	cout << "fragmentOffset = " << iphdr->fragmentOffset << endl;
	cout << "timeToLive = " << iphdr->timeToLive << endl;
	cout << "protocol = " << iphdr->protocol << endl;
	cout << "checksum = " << iphdr->checksum << endl;
	cout << "srcIP = " << (unsigned int)iphdr->srcIP[0] << "-" << (unsigned int)iphdr->srcIP[1] << "-" << (unsigned int)iphdr->srcIP[2] << "-" << (unsigned int)iphdr->srcIP[3] << endl;
	cout << "destIP = " << (unsigned int)iphdr->destIP[0] << "-" << (unsigned int)iphdr->destIP[1] << "-" << (unsigned int)iphdr->destIP[2] << "-" << (unsigned int)iphdr->destIP[3] << endl;

	/*for( int i = 0 ; i < 1480 ; i++ )
	{
		cout << i << "-" << iphdr->payload[i] << endl;
	}
	cout << endl;
	*/
	cout << "payload = " << iphdr->payload[0] << endl;
}

string IPLayer::computeHash(unsigned char* srcIP, unsigned char* destIP, int identification)
{
	string key = "";
	for( int i = 0 ; i < 4 ; i++ )
	{
		key += srcIP[i] + destIP[i];
	}

	key += to_string(identification);

	return key;
}

void IPLayer::insertToMap(string key, IPHeader* iphdr)
{
	if( packetDict.find(key) == packetDict.end() )
	{
		vector<struct packet> v;
		struct packet p;
		p.offset = iphdr->fragmentOffset;

		char* c = (char*) calloc(1480, sizeof(char));
        memcpy(c,iphdr->payload, 1480);

		p.data = c;
		p.MF = iphdr->MF;

		v.push_back(p);
		packetDict[key] = v;
	}
	else
	{
		struct packet p;
		p.offset = iphdr->fragmentOffset;

		char* c = (char*) calloc(1480, sizeof(char));
        memcpy(c,iphdr->payload, 1480);

		p.data = c;
		p.MF = iphdr->MF;

		int size = packetDict[key].size();
		for( int i = 0 ; i < size ; i++ )
		{
			if( packetDict[key][i].offset >  iphdr->fragmentOffset )
			{
				packetDict[key].insert(packetDict[key].begin() + i, p);
				return;
			}
		}

		packetDict[key].push_back(p);
	}
}

void IPLayer::check(string key)
{
	vector<struct packet> v = packetDict[key];
	for( int i = 0 ; i < v.size() ; i++ )
	{
		if( v[i].offset != i*185 ) {
			return;
		}
	}

	if( v.back().MF == 0 )
	{
		int size = v.size();
		size *= 1480;
		char* udpDatagram = (char*)calloc(size, sizeof(char));
		for( int i = 0 ; i < v.size() ;i++ )
		{
			memcpy(udpDatagram+i*1480, v[i].data,1480);
		}

		kernel->udplayer->Receive(udpDatagram);

		packetDict.erase(packetDict.find(key));
	}
}

void IPLayer::Receive(char* data)
{
	IPHeader* iphdr;	
	iphdr = (IPHeader*)data;

	//PrintPacket(iphdr);
	string key = computeHash(iphdr->srcIP, iphdr->destIP, iphdr->identification);
	insertToMap(key, iphdr);
	check(key);
}

void UDPLayer::Send(char* data, int size, char* destIP, int srcPort, int destPort)
{
	const int UDPHEADERSIZE = 8;
	UDPHeader udphdr;
	udphdr.sourcePort = srcPort;
	udphdr.destinationPort = destPort;
	udphdr.checksum = 0;
	udphdr.length = size + UDPHEADERSIZE;

	memset(udphdr.data, '\0', size*sizeof(char));
	memcpy(udphdr.data, data, size*sizeof(char));

	char* buffer = (char*)calloc(size + UDPHEADERSIZE, sizeof(char));
	memcpy(buffer, (const char*)&udphdr, size+UDPHEADERSIZE);


	int dmac = 1;
	if( destIP[0] == 'a' )
		dmac = 0;
	kernel->iplayer->Send(buffer, size + UDPHEADERSIZE, dmac);
}

void printUDPDatagram(UDPHeader* udphdr)
{
	cout << "source port = " << udphdr->sourcePort << endl;
	cout << "Dest port = " << udphdr->destinationPort << endl;
	cout << "length = " << udphdr->length << endl;
	cout << "checksum = " << udphdr->checksum << endl;

	cout << "data = ";
	for( int i = 0 ; i < udphdr->length ; i++ )
		cout << udphdr->data[i];
	
	cout << endl;
}

void UDPLayer::Receive(char* data)
{
	UDPHeader* udphdr;
	udphdr = (UDPHeader*)data;
	
	//printUDPDatagram(udphdr);
	kernel->putData(udphdr->destinationPort, udphdr->data);
}

