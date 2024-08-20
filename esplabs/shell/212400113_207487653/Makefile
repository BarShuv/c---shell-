CC = gcc
CFLAGS = -Wall -g

# Source files for myshell
SRC_MYSHELL = myshell.c LineParser.c

# Source file for mypipeline
SRC_MYPIPELINE = mypipeline.c

# Object files for myshell
OBJ_MYSHELL = $(SRC_MYSHELL:.c=.o)

# Object file for mypipeline
OBJ_MYPIPELINE = $(SRC_MYPIPELINE:.c=.o)

# Target executables
TARGET_MYSHELL = myshell
TARGET_MYPIPELINE = mypipeline

# Default rule to build all targets
all: $(TARGET_MYSHELL) $(TARGET_MYPIPELINE)

# Rule to compile each source file into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files and create the myshell executable
$(TARGET_MYSHELL): $(OBJ_MYSHELL)
	$(CC) $(CFLAGS) $^ -o $@

# Rule to link object files and create the mypipeline executable
$(TARGET_MYPIPELINE): $(OBJ_MYPIPELINE)
	$(CC) $(CFLAGS) $^ -o $@

# Phony target to clean the project directory
clean:
	rm -f $(OBJ_MYSHELL) $(OBJ_MYPIPELINE) $(TARGET_MYSHELL) $(TARGET_MYPIPELINE)
