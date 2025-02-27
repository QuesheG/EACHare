CC = gcc
SRCDIR = src
TARGET = 
ifeq ($(TARGET), server)
SRC = $(filter-out src/client.c, $(wildcard $(SRCDIR)/*.c))
else ifeq ($(TARGET), client)
SRC = $(filter-out src/server.c, $(wildcard $(SRCDIR)/*.c))
endif
FINAL = $(TARGET)
OBJ = $(notdir $(SRC:.c=.o))
HEADER = $(wildcard include/*.h)
# FLAGS = -lpthread
INC = -I include


ifeq ($(OS), Windows_NT)
WFLAG = -lws2_32
else
WFLAG = 
endif

all: $(FINAL)
	del $(OBJ)

$(FINAL): $(OBJ)
	$(CC) $(OBJ) -o $(FINAL) $(FLAGS) $(INC) $(WFLAG)

%.o: $(SRCDIR)/%.c $(HEADER)
	$(CC) -c $< -o $@ $(INC) $(WFLAG)
