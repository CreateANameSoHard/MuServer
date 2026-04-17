CC=g++
CFLAGS=-Wall -g
PFLAGS=-I ../include -L ../lib/ -lhttp -llogger -lsrc -ltimer -lpool -lpthread -lmysqlcppconn
.PHONY: clean all

all:HttpServer EchoServer

HttpServer:
	$(CC) HttpServer.cc $(CFLAGS) -o $@ $(PFLAGS)

EchoServer:
	$(CC) EchoServer.cc $(CFLAGS) -o $@ $(PFLAGS)

clean:
	-@rm -f EchoServer HttpServer