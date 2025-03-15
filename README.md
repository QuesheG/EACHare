# EACHare

EACHare is a file transfer project for USP's Distributed Systems course, it will feature

- Peer to peer connection based on a known list at execution

- File transfer from one peer to another

## Instalation

run
```console
make
```

## Usage

For now, all you have to do is, in your terminal, set up the peer with your ip and a port, choose a text file with your peers (one <ip>:<port> per line) and a directory to share (in the same folder as the peers file)
```console
./eachare <ip>:<port> <peers.txt> <share_folder>
```
the project has examples for both peers.txt and a share_folder
