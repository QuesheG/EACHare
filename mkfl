CC = gcc
CFLAGS = -Wall -Iinclude -MMD -MP
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Arquivos fonte comuns (usando curinga)
COMMON_SOURCES = $(wildcard $(SRC_DIR)/*.c)
# Remove os arquivos específicos do cliente e servidor
COMMON_SOURCES := $(filter-out $(SRC_DIR)/client.c $(SRC_DIR)/server.c, $(COMMON_SOURCES))

# Arquivos fonte específicos
CLIENT_SOURCE = $(SRC_DIR)/client.c
SERVER_SOURCE = $(SRC_DIR)/server.c

# Objetos comuns (gerados a partir dos sources comuns)
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(COMMON_SOURCES))

# Objetos específicos
CLIENT_OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CLIENT_SOURCE))
SERVER_OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SERVER_SOURCE))

# Executáveis
CLIENT_EXE = client
SERVER_EXE = server

ifeq ($(OS), Windows_NT)
LDFLAGS = -lws2_32
REMOVE = del
else
LDFLAGS = 
REMOVE = rm
endif

.PHONY: all client server clean

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_OBJ) $(COMMON_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(SERVER_EXE): $(SERVER_OBJ) $(COMMON_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

# Regra para compilar os objetos
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Cria o diretório build se não existir
$(BUILD_DIR):
	mkdir -p $@

clean:
	REMOVE $(COMMON_OBJS) $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_EXE) $(SERVER_EXE)

-include $(wildcard $(BUILD_DIR)/*.d)