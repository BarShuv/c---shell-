CC = gcc
CFLAGS = -Wall -g

# List of source files
SRC = myshell.c LineParser.c

# List of object files
OBJ = $(SRC:.c=.o)

# Target executable
TARGET = myshell

# Default rule to build the target
all: $(TARGET)

# Rule to compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files and create the target executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Phony target to clean the project directory
clean:
	rm -f $(OBJ) $(TARGET)
