//CISC 450 Project 2 Trevor Roe and Noah Hodgson UDP Transfer
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>

#define IP_PROTOCOL 0
#define IP_ADDRESS "127.0.0.1" // localhost
#define PORT 8080
#define SIZE 82

#define sendrecvflag 0

int packs_received = 0;
int dups_received = 0;
int bytes_received = 0;
int good_acks = 0;
int dropped_acks = 0;
short seq = 0;
int byte_total = 0;

double p_loss_rate;
double ack_loss_rate;
int timeout_val;

//simulate packet loss by using a random float between 0 and 1.
int sim_loss(double loss)
{
	double simulated = (double) (rand()%100) / 100;
	if(simulated < loss){
		//printf("Packet will be lost. \n");
		return 1;
	}
	else{
		//printf("Packet will be successful. \n");
		return 0;
	}
}

//simulate ack loss by using a random float between 0 and 1.
int sim_ack_loss(double loss)
{
	double simulated = (double) (rand()%100) / 100;
	if(simulated < loss){
		//printf("Ack will be lost. \n");
		return 1;
	}
	else{
		//printf("Ack will be successful. \n");
		return 0;
	}
}

// function to clear buffer
void clearBuf(char* b)
{
	int i;
	free(b);
}

//function to strip header information
char* strip_header(char* buffer){
	char* b = (char*) malloc(81*sizeof(char));
	strcpy(b, &buffer[2]);
	return b;
}

// function to receive file
int recvFile(char* buf, int s)
{
	int i;
	char ch;
	printf("\nPacket %d recieved with %d data bytes\n",buf[1], buf[0]+2);
	int count;
	for (i = 2; i < s; i++) {
		ch = buf[i];
		if (ch == EOF)
			return 1;
		else
			count++;
	}
	return 0;
}

// driver code
int main(int argc, char* argv[]){
	//loading in values that are passed in
	if(argc != 4){
		printf("Error, program requires arg for packet loss, ack loss, and timeout value to run.");
		return -1;
	}
	double p_loss_rate = atof(argv[1]);
	double ack_loss_rate = atof(argv[2]);
	int timeout_val = atoi(argv[3]);

	int sockfd, nBytes;
	bool wait; //when waiting for ack
	struct sockaddr_in addr_con;
	int addrlen = sizeof(addr_con);
	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	char net_buf[SIZE]; short ack_buf;
	FILE* fp;
	// socket()
	sockfd = socket(AF_INET, SOCK_DGRAM,
			IP_PROTOCOL);

	if (sockfd < 0)
		printf("\nfile descriptor not received!!\n");
	else
		printf("\nfile descriptor %d received\n", sockfd);
	fp = fopen("out.txt","w"); //create out.txt
	printf("out.txt created.\n");

	while (1) {
		printf("\nPlease enter file name to receive:\n");
		scanf("%s", net_buf);
		sendto(sockfd, net_buf, SIZE,
				sendrecvflag, (struct sockaddr*)&addr_con,
				addrlen);
		//need to wait for ack!
		printf("Sending, stand by for acknowledgement...\n");
		wait = 1;
		while(wait){
			recvfrom(sockfd, &ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);
			if(ack_buf == seq){//we expect the filename ACK to be zero
				wait = 0; //ack_buf = buffer_ack();
			}
		}
		printf("\nfilename ACK successful, ack = %d\n", ack_buf);
		printf("\n---------Data Received---------\n");
		int done_flag = 0;
		while (1) {
			// receive
			bzero(net_buf, SIZE);
			nBytes = recvfrom(sockfd, net_buf, SIZE,
					sendrecvflag, (struct sockaddr*)&addr_con,
					&addrlen);
			// process
			if (recvFile(net_buf, SIZE)) {
				byte_total += net_buf[0] + 4;
				fputs(strip_header(net_buf), fp);
				fclose(fp);
				done_flag = 1;
				break;
			}
			else {
				packs_received++;
				if(net_buf[1] == seq){
					char* readin = (char*) malloc(81*sizeof(char));
					readin = strip_header(net_buf);
					readin[80] = '\0';
					byte_total += net_buf[0] + 4;
					printf("Packet %d delivered to user", seq);
					fputs(readin, fp); //parse datagram
				} else{
					printf("Duplicate packet %d received\n", seq);
					byte_total-=84;
					dups_received++;
				}
				if(!sim_ack_loss(ack_loss_rate)){
					sendto(sockfd, &seq, 1, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);//ack with seq number
					good_acks++;
					printf("\nAck %d generated for transmission\n", seq);
				}
				else{
					printf("ACK %d LOST\n, seq");
					dropped_acks++;
				}
			}
			//else we go here and just send the acknowledgement and don't write to file
		}//loopback to recvfrom
		printf("\n-------------------------------\n");
		if(done_flag){
			break;
		}
	}
	printf("\n===CLIENT TRANSMISSION REPORT===\n");
	printf("All Packets Received: %d\n", packs_received);
	printf("Duplicate Packets Received: %d\n", dups_received);
	int total_pack = packs_received - dups_received;
	printf("Total Packets Minus Duplicates: %d\n", total_pack);
	printf("Total Bytes: %d\n", byte_total);
	printf("Good Ack Total: %d\n", good_acks);
	printf("Dropped Ack Total: %d\n", dropped_acks);
	int total_ack = good_acks + dropped_acks;
	printf("Total Acks: %d\n", total_ack);
	return 0;
}
