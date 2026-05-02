CC = gcc
CFLAGS = -Wall -Wextra -pthread $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -pthread $(shell pkg-config --libs gtk+-3.0) -lm

TARGET = osproject
SRCS = gui.c backend.c ds.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) receipts.txt

.PHONY: all clean
