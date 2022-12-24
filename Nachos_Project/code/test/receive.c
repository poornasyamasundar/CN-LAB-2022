#include "syscall.h"

int main()
{
	int sock;
	char message[20];
	int result; 

	PrintString("Running receive.c\n");
	
	sock = Create_Socket(2,0);
	Bind(sock,1);

	result = Recv_Msg(sock,message, 20);
	if( result == -1 )
	{
		PrintString("Couldn't Receive Message");
	}

	PrintString("Received Message is :");
	PrintString(message);
	PrintString("\n");
	
	Close_Socket(sock);
}
