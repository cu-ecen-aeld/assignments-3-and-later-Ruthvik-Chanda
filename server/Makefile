CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o	

.PHONY : clean
clean :
	rm -f $(aesdsocket) *.o
