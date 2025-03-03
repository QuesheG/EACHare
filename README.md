# DSC

DSC (Distributed System in C) is a file transfer project for USP's Distributed Systems course, it will feature

- Peer to peer connection akin to a bittorrent application

- File transfer akin to a bittorrent application

## Instalation

run
```console
make client
```
to get the client executable
or
```console
make server
```
to get the server executable

## Usage

For now, all you have to do is, in your terminal, set up the server with your ip and a port, and then set up the client with the same port as the server
```console
./server 127.0.0.1:port
```
and
```console
./client 127.0.0.1:port
``` 
