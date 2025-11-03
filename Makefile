CC = gcc
CFLAGS = -Wall -Wextra
TARGET = toralize
SRC = toralize.c

all: $(TARGET)

$(TARGET): $(SRC) toralize.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all

test: $(TARGET)
	@echo "Testing toralize with example.com..."
	./$(TARGET) example.com 80

.PHONY: all clean rebuild test
