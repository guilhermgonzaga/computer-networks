# Computer Networks Assignments

Assignments done for a Computer Networks course at the [UFMS Faculty of Computing](https://facom.ufms.br), Brazil.

`udp-echo` is a simple echo tool to check if a node can be reached. The client will send a UDP message containing one line out of standard input and wait for a response. The server sends back anything received.

`simple-nfs` is a centralized file sharing tool. A connection is established between client and server so that commands and data may be sent to the server. Several operations may be performed on shared files and directories:

- List contents in directories
- Create files and directories
- Upload files to the server
- Delete files individually
- Delete directories recursively

A download feature is missing, as well as a `cd` tool.
