CC = gcc

SRC_DIR = src
OBJ_DIR = build
INC_DIR = include

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

FINAL = eachare

CFLAGS = -Wall -I$(INC_DIR)

ifeq ($(OS), Windows_NT)
LDFLAGS = -lws2_32
REMOVE = del
else
LDFLAGS = 
REMOVE = rm
endif

$(FINAL): $(OBJ_FILES)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	$(REMOVE) $(OBJ_FILES) $(FINAL)