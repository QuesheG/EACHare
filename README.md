# EACHare

EACHare is a file transfer project for USP's Distributed Systems course, it features:

- Peer to peer connection

- Chunk-based file transfer

## Instal

Make sure to have a C compiler and a Make tool, clone the repository and run

```console
make
```

## Using

In the terminal, set up your address, choose a port and a text file with known peers (one <ip>:<port> pair per line) and a directory to share

```console
./eachare <ip>:<port> <neighbours.txt> <shared_directory>
```

This product includes software developed by the Apache Group for use in the Apache HTTP server project (http://www.apache.org/).

## TODO:
- fix thread num (test threads nums and time taken with different files and peers)

- fix error treatment
