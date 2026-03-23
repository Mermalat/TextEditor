CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-result -O2
TARGET = main
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC) editor.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

re: clean all

.PHONY: all clean re
