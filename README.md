# EACHare

EACHare is a file transfer project for USP's Distributed Systems course, it will feature

- Peer to peer connection based on a known list at execution

- File transfer from one peer to another

## Instalation

```console
git clone https://github.com/QuesheG/EACHare
```

run
```console
make
```

## Usage

For now, all you have to do is, in your terminal, set up the peer with your ip and a port, choose a text file with your peers (one <ip>:<port> per line) and a directory to share (in the same folder as the peers file)
```console
./eachare <ip>:<port> <peers.txt> <share_folder>
```

You can also use python to make a testing ground with multiple folders, each with its own known peers and a folder to hold files:
```console
python testing.py
```

## TODO:

Resposta recebida depois de get_peers seguindo relatório

Problema list_peer

Problema build_msg

Problema recebimento

only change status (after GET_PEERS) when receive PEER_LIST