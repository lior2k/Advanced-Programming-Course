
all: threadpool

threadpool: threadpool.o
	gcc threadpool.o -pthread ./libCodec.so -o mythreadpool

threadpool.o: threadpool.c threadpool.h codec.h
	gcc threadpool.c -c

clean:
	rm -f threadpool.o mythreadpool