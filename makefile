all : client.c server.c
	gcc client.c -o client.o
	gcc server.c -o server.o
clean: 
	rm *.o
	rm out.txt
