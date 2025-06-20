# EACHare

EACHare is a file transfer project for USP's Distributed Systems course, it will feature
EACHare é um projeto de transferência de arquivos entre peers para a matéria de Sistemas Distribuídos, implementando:

- Conexão peer to peer baseada em uma lista conhecida em tempo de execução

- Transferência de arquivo entre peers

## Instalação

run
```console
make
```

## Usar

Em seu terminal, monte seu endereço, escolha uma porta, um arquivo de texto com seus peers (um <ip>:<porta> por linha) e um diretório para compartilhar
```console
./eachare <ip>:<porta> <vizinhos.txt> <diretorio_compartilhado>
```

Também é possível usar o python para produzir um campo de testes com múltiplas pastas, cada qual com sua própria lista de peers e diretório para arquivos:
```console
python testing.py
```

This product includes software developed by the Apache Group for use in the Apache HTTP server project (http://www.apache.org/).

## TODO:
- fix append list files same filename

- fix att peer;

- fix thread num (testar numero de threads e tempo com diferentes arquivos e peers)

- fix tratamento de erros

- fix listen_thread mem leak

- fix "port exhaustion" error when sending messages

- fix "not enough space" error when sending messages

src/peer.c:22:    char *tokip = strtok(ip_cpy, ":");
src/peer.c:23:    char *tokport = strtok(NULL, ":");
src/peer.c:99:    strtok(cpy, " ");  // ip
src/peer.c:100:    strtok(NULL, " "); // clock
src/peer.c:101:    strtok(NULL, " "); // type
src/peer.c:102:    strtok(NULL, " "); // size
src/peer.c:103:    char *list = strtok(NULL, "\n");
src/peer.c:124:                p = strtok(cpy_l, " ");
src/peer.c:126:                p = strtok(NULL, " ");
src/peer.c:128:        char *infon = strtok(p, ":");
src/peer.c:144:            infon = strtok(NULL, ":");
src/message.c:426:    char *tok_ip = strtok(buf_cpy, " ");
src/message.c:431:    char *cls = strtok(NULL, " ");
src/message.c:437:    char *tok_msg = strtok(NULL, " ");
src/message.c:859:    strtok(cpy, " ");  // ip
src/message.c:860:    strtok(NULL, " "); // clock
src/message.c:861:    strtok(NULL, " "); // type
src/message.c:862:    strtok(NULL, " "); // size
src/message.c:863:    char *list_rec = strtok(NULL, "\n");
src/message.c:882:                p = strtok(cpy_l, " ");
src/message.c:884:                p = strtok(NULL, " ");
src/message.c:886:        char *infoname = strtok(p, ":");
src/message.c:887:        char *infosize = strtok(NULL, "\0");

