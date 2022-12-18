#include "syscall.h"

int main()
{
	int sock;
	int res;
	char message[100];
	PrintString("Hello from receive function\n");
	PrintString("Received Message is:");

	sock = create_Socket("a", 1, 1);
	recv_msg(sock, message);
	res = close(sock);
	PrintString(message);
	PrintString("\n");
}
