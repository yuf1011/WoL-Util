# Compiler settings
CC = gcc
CFLAGS = -g -Wall

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    TARGET = wol.exe
else
    LIBS =
    TARGET = wol
endif

# Source files
SRC = wol.c
OBJ = $(SRC:.c=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) 