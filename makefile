CC      = g++
CFLAGS  = -std=c++11 -Wall -Wextra
TARGET  = deauther
SRC     = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
