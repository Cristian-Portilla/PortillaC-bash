CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = bash
SRC = PortillaC-bash.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
