#include "syscall.h"

int main()
{
	int sock;
	int res;
	char* message = "Hello World!!";
	PrintString("Hello from Send: \n");
	PrintString("Sending message ");
	PrintString(message);
	sock = create_Socket("b", 1, 1);
	send_msg(sock, message);
	PrintString("\n");
	res = close(sock);
}
