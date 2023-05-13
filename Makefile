CC = gcc
CFLAGS = -Wall

.PHONY: all clean

all: stnc

stnc: stnc.o chat.o performance.o
	$(CC) $(CFLAGS) -o $@ $^ -lcrypto

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o stnc
