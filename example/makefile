CC=g++
CFLAGS=-Wall -g
PFLAGS=-l mymuduo -l pthread -I /usr/include/mymuduo
.PHONY: clean

EchoServer:
	$(CC) EchoServer.cc $(CFLAGS) -o $@ $(PFLAGS)

clean:
	-@rm -f EchoServer