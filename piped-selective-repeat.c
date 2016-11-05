#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef m
#define m 4
#endif

int max_seqno = 15;
int win_size = 8;
int rf=0, rn=8, sf=0, sn=8;

void sender();
void receiver();
int channel[2];

int main(int argc, char const *argv[])
{
	/* code */
	pipe(channel);
	if(fork()==0)
	{
		receiver();
	}
	sender();
	//wait(NULL);
	return 0;
}

void sender() {
	close(channel[0]);
	int data[9];
	int acked[16];
	int i;
	for(i=0; i<8; i++) {
		data[i]=i;
	}
	data[i]=8;
	clock_t t = clock();
	write(channel[1], &data, sizeof(int)*(win_size+1));
	for(i=0; i<8; i++) {
		printf("Sending pkt %d\n", i);
	}
	while( (clock()-t)/CLOCKS_PER_SEC < 10 ) {
		int ack;
		close(channel[1]);
		read(channel[0], &ack, sizeof(int));
		if( (ack<sn && ack>=sf && sf<sn) || (ack<sn && sf>sn) || (ack>=sf &&sf>sn) ) {
			acked[ack]=1;
			printf("Received ACK %d\n", ack);
		}
		if(ack == sf) {
			for(int i=ack; i<ack+win_size; i++) {
				if(acked[i%16]==1) {
					acked[i%16]=0;
					sf = (sf+1)%16;
				}
			}
			printf("Sender Window slided\n");
			printf("Start of s-window: %d\n", sf);
			printf("End of s-window: %d\n", sf+win_size);
			t = clock(); // Restart timer
			int outstanding[win_size+1], k=0;
			for(int j=rf; j<rf+win_size; j++) {
				if(acked[j%16]==0) {
					outstanding[k]=j%16;
					k++;
				}
			}
			outstanding[win_size]=k; // Let the last element be the number of outstanding pkts sent
			close(channel[0]);
			write(channel[1], &outstanding, sizeof(int)*(win_size+1));
			for(int l=0; l<outstanding[win_size]; l++) {
				printf("Sending pkt %d\n", outstanding[l]);
			}
		}
	}
}

void receiver()
{
	int acksent[16];
	for(int i=0; i<16; i++) {
		acksent[i]=0;
	}
	int s1=(unsigned int) time(NULL);
	srand(s1);
	int data[win_size+1];
	close(channel[1]);
	while(1)
	{
		read(channel[0], &data, sizeof(int)*(win_size+1));
		int i;
		for(i=0; i<data[win_size]; i++)
		{
			if(i==3) {
				printf("Lost packet %d\n", data[i]);
			}
			else {
				if( (data[i]<rn && data[i]>=rf && rf<rn) || (data[i]<rn && rf>rn) || (data[i]>=rf && rf>rn) ) {
					printf("Received pkt %d\n", data[i]);
					int ack2send = data[i];
					close(channel[0]);
					write(channel[1], &ack2send, sizeof(int));
					acksent[data[i]]=1;
					printf("Sent ACK %d\n", data[i]);
					if(ack2send == rf) {
						for(int i=ack2send; i<ack2send+win_size; i++) {
							if(acksent[i%16]==1) {
								acksent[i%16]=0;
								rf = (rf+1)%16;
							}
						}
						printf("Receiver Window slided\n");
						printf("Start of r-window: %d\n", rf);
						printf("End of r-window: %d\n", rf+win_size);
					}
				}
			}
		}
	}
}