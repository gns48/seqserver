CFLAGS =  -I. -Wall -pedantic -g
LIBS = -levent
CC = gcc

TARGET = seqserver
SRC = seqserver.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o a.out *~ $(TARGET) core

seqserver.c: seqserver.h
