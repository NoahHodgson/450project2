// server code for UDP socket programming
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

int datapacket_num = 0;
int bytes_transmitted = 0;
int packets_transmitted = 0;
int suc_packets = 0;
int dropped_packets = 0;
int ack_count = 0;
int timeout_count = 0;
bool seq = 1;

double p_loss_rate;
double ack_loss_rate;
int timeout_val;

bool invoke_seq(){
	seq = !seq;
	return seq;
}

short buffer_ack(){ //seq flips every time starting at 1, so first return (for filename ack) is 0
	invoke_seq();
	if(seq == true){
	    return 1;
	} else {
	    return 0;
	}
//printf("seq: %d\n",seq);
}

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
	buf[1] = invoke_seq();
    return 0;
}
// driver code
int main(int argc, char* argv[])
{
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
	double p_loss_rate = atof(argv[0]);
	double ack_loss_rate = atof(argv[1]);
	int timeout_val = atoi(argv[2]);


    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);

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
        clearBuf(net_buf);
        nBytes = recvfrom(sockfd, net_buf,
                          SIZE, sendrecvflag,
                          (struct sockaddr*)&addr_con, &addrlen);

	if(nBytes > 0){ //if we recieve name, we need to ack with 0!
	    	ack_buf = buffer_ack();
		printf("filename acK: %d \n", ack_buf);
		sendto(sockfd, &ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
	}

        fp = fopen(net_buf, "r");
        printf("\nFile Name Received: %s\n", net_buf);
        if (fp == NULL)
            printf("\nFile open failed!\n");
        else
            printf("\nFile Successfully opened!\n");

        while (1) {
            // process
            if (sendFile(fp, net_buf, SIZE)) {
            	if(!sim_loss(p_loss_rate)){
			printf("EOF reached\n");
            		sendto(sockfd, net_buf, SIZE, sendrecvflag, (struct sockaddr*)&addr_con, addrlen);
            		packets_transmitted++;
                	break;
            	}else{
            		printf("Packet Lost!\n");
            		dropped_packets++;
            	}
            }

            // send
            if(!sim_loss(p_loss_rate)){
		printf("Enter send conditional\n");
            	sendto(sockfd, net_buf, SIZE,sendrecvflag,(struct sockaddr*)&addr_con, addrlen);
		printf("datagram transfer complete\n");
		printf("waiting for ack w/ seq: %d", seq);
		wait = 1;
		//clearBuf(net_buf);
		//printf("Waiting for datagram ack w/ seq: %d", seq);
		while(wait){ //wait for ack
		    printf("\nwaitloop\n");
		    recvfrom(sockfd, &ack_buf, 1, sendrecvflag, (struct sockaddr*)&addr_con, &addrlen);
		    if(ack_buf == (char)seq){ 
			wait = 0; 
			printf("\n DATAGRAM ACK RECIEVED\n");
		    }
		}
		clearBuf(net_buf);
            	packets_transmitted++;
        	}else{
        		printf("Packet Lost!\n");
        		dropped_packets++;
        	}
        }
        if (fp != NULL)
            fclose(fp);
    }
    //printing required values
    printf("\n===SERVER TRANSMISSION REPORT===\n");
    printf("Datapacket total: %d\n", datapacket_num);
    printf("Byte total: %d\n", bytes_transmitted);
    printf("Transmitted packets total: %d\n", packets_transmitted);
    printf("Total dropped packets: %d\n", dropped_packets);
    printf("Total successful packets: %d\n", suc_packets);
    printf("Number of received acks: %d\n", ack_count);
    printf("Number of timeouts: %d\n", timeout_count);
    return 0;
}
