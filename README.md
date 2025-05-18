# EACHare

EACHare is a file transfer project for USP's Distributed Systems course, it will feature
EACHare é um projeto de transferência de arquivos entre peers para a matéria de Sistemas Distribuídos, implementando:

- Conexão peer to peer baseada em uma lista conhecida em tempo de execução

- Transferência de arquivo entre peers

## Instação

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
Requisitos EP3
Testes
Cheque teste do prof (obter peers -> listar peers erro)
Falha lendo arquivos após recebimento
