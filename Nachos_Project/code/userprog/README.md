##System Call Implementation:
The final part of the implementation is System call implementation to enable user programs to use the built network stack.
We are going to implement four system calls. They are:
* **int Create_Socket(int type, int protocol)**
	This function will create a new socket and returns the sock ID as output. The inputs are:
	* type
		communication type: 2 for SOCK_DGRAM
	*  protocol
		0 for IP Protocol.
* ** int Close_Socket(int sockID)**
	Closes the socket with the given *sockID*.
* **int Bind(int sockID, int port)**
	Binds the given socket to the given port number.
* **int Connect(int sockID, char* ip, int port)**
	Connects the given socket to the given ip address and port number.
* **int Send_Msg(int sockID, char* msg)**
	Sends the given message(*msg*) through the given socket(*sockID*).
* **int Recv_Msg(int sockID, char* msg, int size)**
	Receives a message through *sockID8, and copies the received message in the space pointed by *msg*. if the received message is more than *size*, then the this function copies the first *size* number of bytes.

We will declare new interrupt numbers for these system calls and provide their declarations in [syscall.h](/Nachos_Project/code/userprog/syscall.h).
Also, edit Start.S to include these interrupt handling destinations.

The Interrupt handler functions are provided in [exception.h](/Nachos_Project/code/userprog/exception.h).
All the variables defined below will be stored in kernel. Helper functions in [kernel.h](/Nachos_Project/code/threads/kernel.h) will be used to handle the exceptions.
####Create_Socket
We maintain a variable called ***current_socket*** intialized with 1. Each time a call to *Create_Socket* is made the *current_socket* is returned and then incremented.

####Connect
We shall maintain a map called ***connected_sockets*** with key as *sockID*, and value as ip and port pair. Whenever a function call to *Connect* is made, we insert a new key-value pair to the this map.

####Send_Msg
With the help of *sockID* and *msg*(received as arguments) and the *connected_sockets* map, we will call the *UDP_Layer.Send()* function

####Bind
We will maintain an other map called ***bind_sockets*** with key as port number and value as socket number. Calling *Bind* will check if the port is already binded or not. If it is binded, then we return -1. Else we insert a new pair.

####Recv_msg
For now, we are just printing the packets in *UDP_Layer.Receive()*. Now, we will insert the received packets into another new map called ***datagrams***, whose key is the destination port number of the *UDP_Datagram*, and the value is a queue. This queue contains the data packets received to this destination port. When a call to *Recv_msg* is made, we find the port number from *bind_sockets* map, and then pop the element from the corresponding queue and return it. If the queue is empty we wait. *Synchlist* will used to implement this queue.

####Close_Socket
Closing a socket is nothing but deleting any entries in the above created maps with ids as sockID.

The next part is [here](/Nachos_Project/code/test/README.md).
