##Link Layer.
Since, we deleted the contents of post.cc and post.h, we also have to delete all the code where the deleted functions are used.
Delete the below lines:
* Contents of NetworkTest function in [kernel.cc](/Nachos_project/code/threads/kernel.cc)
* Class declarations of PostOfficeInput and PostOfficeOutput in line 22 and 23 , and class instance in line 62 and 63 in [kernel.h](/Nachos_project/code/threads/kernel.h)
* Instantiations in line 107, 108, 133, and 134 in [kernel.cc](/Nachos_project/code/threads/kernel.cc)

Now, all the references are deleted. We can verify this by running `make nachos` in [build-linux](/Nachos_project/code/build-linux)( after every change run 'make nachos', this will compile the entire code again) and see that there are no errors while compiling.

Lets start by declaring the Ethernet packet structure. The Ethernet packet structure is defined in [post.h](/Nachos_project/code/network/post.h) as a struct variable called ***Ethernet_Packet***.
Also, declare a ***macpool***, with random mac values. Nachos instance that is declared with machine address i will use ith mac address from this *macpool*.

Create a new class called ***Ethernet_Layer***, with two main functionns ***Send*** and ***Receive***, import necessary header files in [post.h](/Nachos_project/code/network/post.h).

Now, these functions will send and receive data from the files through the *NetworkInput* and *NetworkOutput* modules, which are declared in [network .h](/Nachos_project/code/machine/network.h) ( Everything in machine folder are responsible for simulating an actual machine, in our case [network .ccc](/Nachos_project/code/machine/network.cc) simulates the Network Card in an actual computer). So, when we call *networkOut->send*, we can think of it as sending a packet through the Ethernet port of an actual computer.

The implementation of ***Ethernet_Layer.Send()*** is simple, just create a new *Ethernet_Packet*, fill all the fields with the given arguments, place all 0's for fields that we don't use( like preamble, sfd, crc) ( In our implementation, we are getting the destination MAC directly as input, but originally we only get the destination IP, then the link layer has to run **ARP Protocol** to find the destination MAC, which we are skipping in this implementation, we got ARP implementation in our final lab exam)


We also declare a *sendlock* in *Ethernet_Layer*, so that only one packet is sent through the network at any time. we acquire the lock before calling *networkOut->send*, and release after it. We send the packet by serializing it into a char array.

Also, previously *networkOut->send* takes *Packet_Header* as input, since, we no longer use packet and mail headers, we will make the necesssary changes.

The Receiving side is a bit trickier, we have to create separate thread called *pktrecv*, which will continuously see if there is any packet to receive or not. We will use nachos *Threads, Locks, Semaphores and SynchLists* to achieve this. Once we got something to read, then this thread will read that message and put that in a queue( a SynchList ).( This *pktrecv* thread is similar to the *PostalDelivery* thread in the original *PostOfficeOut* implementation ).

***Ethernet_Layer.Receive()*** will get the top most element from this queue and return it. If the queue is empty, then it will wait until the queue is not empty(the SynchList will automatically do the waiting).

Now, who will call *Ethernet_Layer.Recieve()*? We will again create another thread called *ethrecv*, which will continuously run and call *Ethernet_Layer.Receive()* and gets the packet and sends it to higher layers.

>Why is the need for an intermediate queue? why can't we immediately call *IP_Layer.Receive()* as soon as we get a packet in pktrec thread. The answer is performance. Processing a packet by higher layers is a time consuming task. If we call IP_Layer.Receive() directly from pktrecv, then pktrecv clockcycles of pktrecv will be consumed while processing the packet all the way up to UDP, meanwhile the incoming packets might get lost if the receiving thread is busy( Remember that there is no queue in network.cc, irrespective of the current_packet being received or not by Ethernet_layer, NetworkInput will override the existing packet if new packet arrives). 

Also, few changes need to be made in NetworkInput just like for NetworkOutput to accomodate the new network stack.
Now, we have to instantiate the *Ethernet_Layer* in [kernel.cc](/Nachos_project/code/threads/kernel.cc).

At this point, both send and receive part of the Link Layer is done. We can test this by calling the *Send* function from *NetworkTest* in  [kernel.cc](/Nachos_project/code/threads/kernel.cc) using the above instantiated *Ethernet_Layer*. Just print the payload at the *Ethernet_Layer.Receive()* function. For receiving to happen we need to keep the nachos instance alive to do this yield the current thread in an infinite while loop in [main.cc](/Nachos_project/code/threads/main.cc)

##Network Layer
Again declare an ***IP_Fragment*** structure, define an ***ippool***( just like macpool ), and an ***IP_Layer*** class with Send and Receive functions in [post.h](/Nachos_project/code/network/post.h).

***IP_Layer.Send()*** is simple. The size of input data can be as much as 65KB. Fragment them into chunks of 1480 bytes( the last chunk can be less than 1480 bytes), append each chunk with the 20 byte IP Header, fill all fields. The More Fragments(MF) bit will be 1 for every fragment except the last one. This process is called fragmentation. For each fragment call *Ethernet_Layer.Send()* function.

Receiving side is again a bit tricky. The *ethrecv'* thread will call ***IP_Layer.Receive()*** with the *payload* of *Ethernet_Packet* as input. We maintain a map called *fragmentDict*, whose key is a combination of *srcIP, destIP, and IdentificationNumber*  and the map value is a vector of all fragments with the given key sorted according to *fragmentOffset*. After receiving each fragment, we check if all the fragments corresponding to the same key are received or not. After receiving all fragments we call *UDP_Layer().Receive()*.

##Transport Layer:
As usual declare a ***UDP_Datagram*** structure and a class ***UDP_Layer*** with ***Send*** and ***Receive*** functions in [post.h](/Nachos_project/code/network/post.h).

***UDP_Layer.Send()*** will get the data as input along with destination port, source port and destination IP. Fill the *UDP_datagram* with these inputs and call *IP_Layer.Send()*.

***UDP_Layer.Receive()*** for now will simply print the received packet. In the Next step, when the system calls are implemented we will again update the *UDP_Layer.Receive()* function.

System Call implementation is [here](/Nachos_project/code/userprog/README.md).

