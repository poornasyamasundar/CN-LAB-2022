To install nachos just clone this repository and run  `bash full_install.sh` in the nachos-project directory.
Nachos already has a mini network stack through which communication happens between two nachos instances. This mini stack has just two layers. Also, The functionality of the network stack is within the os, and it is not available for user programs. This means there is no system call implementation of this network service through which the user can send and receive messages.

A demo of this network stack can be seen by opening two terminals and changing the current directory in both the terminals to build.linux and run these two commands.
In the first terminal run this command: `./nachos -m 1 -N`
In the second terminal run this command:`./nachos -m 0 -N`

We can see 'Hello there!', 'Got it!' messages getting  printed on both the terminals.
> The -m flag is used to give the nachos instances their machine ids. By default this value is 0. The -N flag is used to run a network test. Code that printed the above messages can be found at [kernel.cc](/Nachos_project/code/threads/kernel.cc).

This NetworkTest is simply using the *postofficeOut.send* and *postofficeIn.receive* to send and receive messages.
The entire documentation of this mini stack is in [post.cc](/Nachos_project/code/network/post.cc) and [post.h](/Nachos_project/code/network/post.h). The description of this implementation can be found in the [Nachos roadmap](https://users.cs.duke.edu/~narten/110/nachos/main/main.html).

Since, we are developing our own stack, we will remove the entire contents of [post.cc](/Nachos_project/code/network/post.cc) and [post.h](/Nachos_project/code/network/post.h).( don't delete the files, just make them empty).

We followed the bottom up approach in the lab while implementing the network stack. We implemented the entire stack in the following order:
* Switch(which is outside of nachos)
* Link layer(ETHERNET)
* Network layer(IP)
* Transport layer(UDP)
* System call implementation and user programs.
The switch implementation is [here](/Switch/README/md), network stack implementation is [here](/Nachos_project/code/network/README.md), and the system call implementation is [here](/Nachos_project/code/userprog/README.md).
