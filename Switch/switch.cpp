#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<unistd.h>
#include<netinet/in.h>
#include<iostream>
#include<bits/stdc++.h>
#include"ethernet.h"
using namespace std;

#define switch0 "/tmp/switch0"
#define switch1 "/tmp/switch1"
#define switch2 "/tmp/switch2"
#define switch3 "/tmp/switch3"

#define machine0 "/tmp/machine0"
#define machine1 "/tmp/machine1"
#define machine2 "/tmp/machine2"
#define machine3 "/tmp/machine3"

vector<int> switch_ports;
vector<int> machine_ports;

vector<struct sockaddr_un> switchAddresses;
vector<struct sockaddr_un> machineAddresses;

unordered_map<string, int> switchMap;

string getString(Ethernet* eth, bool b)
{
	string s = "";
	for( int i = 0 ; i < 5 ; i++ )
	{
		if( b )
		{
			s += to_string(int(eth->destMAC[i])) + "-";
		}
		else
		{
			s += to_string(int(eth->srcMAC[i])) + "-";
		}
	}
	if( b )
	{
		s += to_string(int(eth->destMAC[5]));
	}
	else
	{
		s += to_string(int(eth->srcMAC[5]));
	}
	
	return s;
}

vector<int> Get_Ports()
{
	vector<int> ports(4, 0);

	for( int i = 0 ; i < 4 ; i++ )
	{
		ports[i] = socket(AF_UNIX, SOCK_DGRAM, 0);
		if( ports[i] <= 0 )
		{
			cout << "port " << i << " cannot be created" << endl;
			exit(0);
		}
	}

	return ports;
}

vector<struct sockaddr_un> Set_Switch_Addresses()
{
	vector<struct sockaddr_un> addresses(4);

	unlink(switch0);
	unlink(switch1);
	unlink(switch2);
	unlink(switch3);

	for( int i = 0 ; i < 4 ; i++ )
	{
		struct sockaddr_un address;
		memset(&address, 0, sizeof(address));
		address.sun_family = AF_UNIX;
		switch(i)
		{
			case 0:
				strcpy(address.sun_path, switch0);
				break;
			case 1:
				strcpy(address.sun_path, switch1);
				break;
			case 2:
				strcpy(address.sun_path, switch2);
				break;
			case 3:
				strcpy(address.sun_path, switch3);
				break;
		}

		int rc = bind(switch_ports[i], (struct sockaddr*)&address, SUN_LEN(&address));

		if( rc < 0 )
		{
			cout << "Binding failed" << endl;
			exit(0);
		}
	}
	return addresses;
}

vector<struct sockaddr_un> Set_Machine_Addresses()
{
	vector<struct sockaddr_un> addresses(4);
	
	for( int i = 0 ; i < 4 ; i++ )
	{
		struct sockaddr_un address;
		memset(&address, 0, sizeof(address));
		address.sun_family = AF_UNIX;
		switch(i)
		{
			case 0:
				strcpy(address.sun_path, machine0);
				break;
			case 1:
				strcpy(address.sun_path, machine1);
				break;
			case 2:
				strcpy(address.sun_path, machine2);
				break;
			case 3:
				strcpy(address.sun_path, machine3);
				break;
		}

		int client_fd;
		
		if( (client_fd == connect(machine_ports[i], (struct sockaddr*)&address, sizeof(address))) < 0 )
		{
			cout << "Connecting to machine " << i << " failed" << endl;
		}
	}

	return addresses;
}

void clean()
{
	for( int i = 0 ; i < 4 ; i++ )
	{
		close(switch_ports[i]);
		close(machine_ports[i]);
	}
}

void printMessage(Ethernet* eth)
{
	cout << "Received message is : " << endl;
	for( int i = 0 ; i < 1500 ; i++ )
	{
		cout << eth->payload[i];
	}
	cout << endl;
	cout << "Received From: " << getString(eth,false) << endl;
	cout << "Sent To: " << getString(eth, true) << endl;
}

int main()
{
	switch_ports = Get_Ports();
	machine_ports = Get_Ports();

	switchAddresses = Set_Switch_Addresses();

	while(true)
	{
		fd_set rd;
		int maxfd = switch_ports[0];
		FD_ZERO(&rd);
		for( int i = 0 ; i < 4 ; i++ )
		{
			maxfd = max(maxfd, switch_ports[i]);
			FD_SET(switch_ports[i], &rd);
		}

		int active_fd = select(maxfd+1, &rd, NULL, NULL, NULL);
		for( int i = 0 ; i < 4 ; i++ )
		{
			if( FD_ISSET(switch_ports[i], &rd) != 0 )
			{
				cout << "Got something at switch " << i << endl;
				unsigned char buffer[1526];
				recv(switch_ports[i], &buffer, 1526, 0);

				Ethernet* eth;
				eth = (Ethernet*)buffer;
				printMessage(eth);
				
				machineAddresses = Set_Machine_Addresses();

				switchMap[getString(eth, false)] = i;

				string dest = getString(eth, true);

				if( dest == "255-255-255-255-255-255" )
				{
					cout << "this is a broad cast message, So, sending to all machines" << endl;
					for( int j = 0 ; j < 4 ; j++ )
					{
						if( j != i )
						{
							send(machine_ports[j], buffer, 1526, 0);
						}
					}
				}
				else if( switchMap.find(dest) != switchMap.end() )
				{
					cout << "Sending to port " << switchMap[dest] << endl;
					send(machine_ports[switchMap[dest]], buffer, 1526, 0);
				}
				else
				{
					cout << "Sending to all ports" << endl;
					for( int j = 0 ; j < 4 ; j++ )
					{
						if( j != i )
						{
							send(machine_ports[j], buffer, 1526, 0);
						}
					}
				}
					cout << endl;
					cout << endl;
			}
		}
	}
	
	clean();
}
