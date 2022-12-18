#ifndef _UDP_H
#define _UDP_H

struct UDPHeader{
	unsigned short int sourcePort;
	unsigned short int destinationPort;
	unsigned short int length;
	unsigned short int checksum;
	char data[65535];
}__attribute__((packed));	
#endif


