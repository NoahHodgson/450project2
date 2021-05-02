all : client.c server.c
	gcc client.c -o client.o
	gcc server.c -o server.o
clean: 
	rm *.o
	rm out.txt

client: client.o
	./client.o 0 0 0

server: server.o
	./server.o 0 0 0
