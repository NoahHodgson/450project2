//CISC 450 Project 2 Trevor Roe and Noah Hodgson UDP Transfer
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define IP_PROTOCOL 0
#define PORT 8080
#define SIZE 82

#define sendrecvflag 0
#define nofile "File Not Found!"

int init_datapacket_num = 0;
int bytes_transmitted = 0;
int ack_count = 0;
int retrans = 0;
int ploss = 0;
int successes = 0;
int timedout = 0;
short seq = 0;

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
	for (i = 0; i < SIZE; i++)
		b[i] = '\0';
}

char* strip_header(char* buffer){
	char* b = (char*) malloc(81*sizeof(char));
	strcpy(b, &buffer[2]);
	return b;
}

// function sending file
int sendFile(FILE* fp, char* buf, int s)
{
	int i, len;
	if (fp == NULL) {//if no fp to copy from, nofile = "cannot find file"
		printf("Server located null file.\n");
		strcpy(buf, nofile);
		len = strlen(nofile);
		buf[len] = EOF;
		return 1;
	}
	//fill buffer with data
	int16_t count = 0; //data count
	char ch, ch2;
	for (i = 2; i < s; i++) {
		ch = fgetc(fp);
		buf[i] = ch;
		count++;
		if (ch == EOF)
			return 1;
	}
	//add header in first 2 indices
	buf[0] = count;//each char is 1 byte
	buf[1] = seq; //flip every time
	printf("Packet %d generated with %d data bytes\n", buf[1], buf[0]);
	bytes_transmitted += (count + 4); //"four" header bytes plus datagram
	return 0;
}
// driver code
int main(int argc, char* argv[])
{
	srand(time(0));
	int sockfd, nBytes;
	bool wait; //for use when waiting for ack's
	struct sockaddr_in addr_con;
	int addrlen = sizeof(addr_con);
	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;
	char net_buf[SIZE]; short ack_buf;
	FILE* fp;
	//loading in values that are passed in
	if(argc != 4){
		printf("Error, program requires arg for packet loss, ack loss, and timeout value to run.");
		return -1;
	}
	double p_loss_rate = atof(argv[1]);
	double ack_loss_rate = atof(argv[2]);
	int timeout_val = atoi(argv[3]);

	// socket()
	sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);

	//timeout
	struct timeval timeout;
	timeout.tv_sec = timeout_val;
	timeout.tv_usec = 0;
	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));



	if (sockfd < 0)
		printf("\nfile descriptor not received!!\n");
	else
		printf("\nfile descriptor %d received\n", sockfd);

	// bind()
	if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0)
		printf("\nSuccessfully binded!\n");
	else
		printf("\nBinding Failed!\n");


	while (1) {
		printf("\nWaiting for file name...\n");

		// receive file name
		clearBuf(net_buf); wait = 1;
		while(wait){
			nBytes = recvfrom(sockfd, net_buf,
					SIZE, sendrecvflag,
					(struct sockaddr*)&addr_con, &addrlen);

			if(nBytes > 0){ //if we recieve name, we need to ack with 0!
				wait = 0;
				printf("filename ack: %d \n", ack_buf);
				sendto(sockfd, &ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
			}
		}
		fp = fopen(net_buf, "r");
		printf("\nFile Name Received: %s\n", net_buf);
		if (fp == NULL)
			printf("\nFile open failed!\n");
		else
			printf("\nFile Successfully opened!\n");
		ack_count++;
		int done_flag=0;
		clearBuf(net_buf);
		while (1) {
			// process
			if (sendFile(fp, net_buf, SIZE)) {
				successes++;
				printf("EOF reached, seq: %d\n", seq);
				printf("%s \n", strip_header(net_buf));
				wait = 1;
				sendto(sockfd, net_buf, SIZE, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
				done_flag = 1;
				break;
			}
			init_datapacket_num++;
			wait = 0;
			int flag=0;
			while(!wait){
				if(!sim_loss(p_loss_rate)){
					printf("%s \n", strip_header(net_buf));
					sendto(sockfd, net_buf, SIZE,sendrecvflag,(struct sockaddr*)&addr_con, addrlen);
					printf("Packet %d successfully transmitted with %d bytes\n", seq, sizeof(net_buf));
					printf("waiting for ack w/ seq: %d\n", seq);
					successes++;
				}else{
					printf("Packet %d Lost!\n", seq);
					ploss++;
				}
				int timeout = recvfrom(sockfd, &ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);
				if(timeout<0){//if NO ACK
					printf("\nTimeout expired for packet numbered %d\n", seq);//timeout waiting for ack
					int count;
					for (int i = 2; i < SIZE; i++) {
						char ch = fgetc(fp);
						count++;
						if (ch == EOF){
							break;
						}
					}
					bytes_transmitted -= (count + 4);
					retrans++;
					timedout++;
					int goback = count%80;
					if (goback == 0){goback=80;}
					printf("go back: %d\n\n", count);
					if(!flag && goback != 80){
						fseek(fp, -goback+1, SEEK_CUR);
						flag=1;
					}
					else if(!flag && goback == 80){
						fseek(fp, -goback, SEEK_CUR);
					}
				}else{ //otherwise YES WE GOT AN ACK
					wait = 1;
					printf("\nDATAGRAM ACK %d RECIEVED\n", seq);
					//go sequence number
					seq=1-seq;
					ack_count++;
					clearBuf(net_buf);
				}

			} if(done_flag){ break; }
		}
		if (fp != NULL)
			fclose(fp);
		if(done_flag){
			break;
		}
	}
	//printing required values
	printf("\n===SERVER TRANSMISSION REPORT===\n");
	printf("Initial datapacket total: %d\n", init_datapacket_num);
	printf("Databytes generated for original transmission: %d\n", bytes_transmitted);
	int retrans_full = retrans + init_datapacket_num;
	printf("Retransmitted packets (retrans+init) generated: %d\n", retrans_full);
	printf("Number of Lost Packets: %d\n", ploss);
	printf("Number of Successful Transmissions: %d\n", successes);
	printf("Number of ACKs: %d\n", ack_count);
	printf("Number of Timeouts: %d\n", timedout);
	return 0;
}
