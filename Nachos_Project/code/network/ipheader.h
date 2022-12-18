#ifndef _IP_H
#define _IP_H

struct IPHeader{
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
#endif
