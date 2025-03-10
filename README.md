# DSC

DSC (Distributed System in C) is a file transfer project for USP's Distributed Systems course, it will feature

- Peer to peer connection based on a known list at execution

- File transfer from one peer to another

## Instalation

run
```console
make
```

## Usage

For now, all you have to do is, in your terminal, set up the peer with your ip and a port, choose a text file with your peers and a directory to share
```console
./eachare <ip>:<port> <peers.txt> <share_folder>
```
the project has examples for both peers.txt and a share_folder