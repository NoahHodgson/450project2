// client code for UDP socket programming
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
bool seq;

double p_loss_rate;
double ack_loss_rate;
int timeout_val;

//simulate packet loss by using a random float between 0 and 1.
int sim_loss(double loss)
{
    double simulated = (double) (rand()%100) / 100;
    if(simulated < loss){
        printf("Packet was lost. \n");
        return 1;
    }
    else{
        printf("Packet successfully transferred. \n");
        return 0;
    }
}

//simulate ack loss by using a random float between 0 and 1.
int sim_ack_loss(double loss)
{
    double simulated = (double) (rand()%100) / 100;
    if(simulated < loss){
            printf("Packet was lost. \n");
            return 1;
        }
        else{
            printf("Packet successfully transferred. \n");
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

//function to strip header information
char* strip_header(char* buffer){
	for(int i=0;i<SIZE-2;i++){
	    buffer[i] = buffer[i+2];}
	buffer[SIZE-1] = '\0';
	buffer[SIZE] = '\0';
    return buffer;
}

// function to receive file
int recvFile(char* buf, int s)
{
    int i;
    char ch;
	printf("\nPacket size: %d\t, seq: %d\n",buf[0], buf[1]);
//	buf[0] = ""; buf[1] = "";
    for (i = 2; i < s; i++) {
        ch = buf[i];
        if (ch == EOF)
            return 1;
	else if (ch == '\0')
	    printf("%s", "/0");
	else
	    printf("%c",ch);
    }       //printf("%c", '\t');//tab between each recv'd packet
	    //printf("\nLast char is %c\n", buf[s]);
    return 0;
}
  
// driver code
int main(int argc, char* argv[]){
	//loading in values that are passed in
	if(argc != 4){
		printf("Error, program requires arg for packet loss, ack loss, and timeout value to run.");
		return -1;
	}
	double p_loss_rate = atof(argv[0]);
	double ack_loss_rate = atof(argv[1]);
	int timeout_val = atoi(argv[2]);

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
	    if(ack_buf < 1){
	        wait = 0;
	    }
	}
	printf("ACK successful");
        printf("\n---------Data Received---------\n");

        while (1) {
            // receive
            bzero(net_buf, SIZE);
            //if not ackloss goes here around recvfrom func
            nBytes = recvfrom(sockfd, net_buf, SIZE,
                              sendrecvflag, (struct sockaddr*)&addr_con,
                              &addrlen);
            // process
            if (recvFile(net_buf, SIZE)) {
            	//net_buf = strip_header(net_buf);
            	fprintf(fp, strip_header(net_buf));
            	fclose(fp);
                break;
           } else {
		//net_buf = strip_header(net_buf);
		ack_buf = 1;//net_buf[1]; //pull seq id
	        fprintf(fp, strip_header(net_buf)); //parse datagram
		sendto(sockfd, ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);//ack with seq number
	    }
     }
        printf("\n-------------------------------\n");
  }
    printf("\n===SERVER TRANSMISSION REPORT===\n");
    printf("Packets Received: %d\n", packs_received);
    printf("Duplicate Packets Received: %d\n", dups_received);
    int total_pack = packs_received + dups_received;
    printf("Total Packets: %d\n", total_pack);
    printf("");
    return 0;
}
