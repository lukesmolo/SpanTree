#SpanTree: emulating Spanning Tree algorithm from CLI

SpanTree is a rapid and efficient way for emulating the spanning tree algorithm,
using UDP datagrams as messages exchanged among bridges.<br />
You have just to write your configuration file and then run the program for discovering how network will be configured.<br />
So far, the maximum number of bridges accepted is **5**, the maximum number of different LANs is **12**.

##Configuration File
Each Bridge is represented by two characters and a number, each LAN by an unique
capital letter.

Configuration file has to be written in the following way:

Bridge**|**LAN-LAN-LAN<br />
Bridge**|**LAN-LAN-LAN

For instance, a valid configuration file could be:
```
B1|A-B-C-D-E-F-G-H-I-L-W
B2|A-B-C-D-E-F-G-H-I
B3|A-B-C-D-E-F
```

##Usage
Clone the repository:
```
$ git clone --recursive git@github.com:lukesmolo/SpanTree.git
```
Compile the program:
```
$ make
```
Run the program:
```
$ ./main
```


##What else?
SpanTree is my project for _Networking_ undergraduate course in **2013**, so I had to follow specific [design choices (Italian only)](/docs/SpanTree.pdf).

##License
SpanTree is released under the GPLv2.

##TODO
* accept more than **5** bridges and more than **12** LANs.

