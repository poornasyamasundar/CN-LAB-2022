#include "syscall.h"

int main()
{
	int sock;
	int result;
	char* message = "Hello World!!";
	char* ip = "174.50.24.217";

	PrintString("Running send.c\n");
	PrintString("Sending message = ");
	PrintString(message);
	PrintString("\n");
	
	sock = Create_Socket(2,0);
	Connect(sock, ip, 1, 1);

	result = Send_Msg(sock,message);
	if( result == 1 )
	{
		PrintString("Message sent\n");
	}
	else
	{
		PrintString("Message not sent\n");
	}
	Close_Socket(sock);
}
