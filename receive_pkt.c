#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define UDP_IP "10.0.1.38"
#define UDP_PORT 10000
#define SIZE 100000
#define FRAME 768
void main(){
        int serverSocket;

        struct sockaddr_in serverAddr;
        // 建立Socket，并设置
        serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
        // 设置socket选项，这是可选的，可以避免服务器程序结束后无法快速重新运行
        int val=1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        // 定义端口地址
        bzero(&serverAddr,sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(UDP_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(UDP_IP);
        int rc = bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (rc == -1) {
         printf("Bad bind\n");
        }
        int i,j,mcnt;
        //int recv_len;
        int start=time((time_t*)NULL);
        FILE *sfp;
        //sockaddr_in remoteAddr;
        unsigned int serverAddr_len=sizeof(serverAddr);
        printf("serverAddr_len is %d\n",serverAddr_len);
        char *data;
        data = (char *) malloc (sizeof(char)*FRAME*SIZE);
	unsigned char frm_header[8]; //char header information from packet
        unsigned long header; // for 64 bit counter     
        unsigned long SEQ=0;
        unsigned long LAST_SEQ=0;
        unsigned long CHANNEL;
    	//unsigned char *frm_data;
    	//frm_data = (unsigned char *)malloc(PKTSIZE*sizeof(unsigned char));

        //printf("file name is: %s\n",filename);
        sfp=fopen("test171019.dat","wb");
        for(i=0;i<SIZE;i++){
                mcnt=recvfrom(serverSocket,data,FRAME,0,(struct sockaddr*)&serverAddr, &serverAddr_len);
	        for(int j=0;j<8;j++){
	                        frm_header[j]=data[j*3];
	                }
	
	        memcpy(&header, frm_header,8*sizeof(unsigned char));
	        CHANNEL = header & 0x3fff;
	        SEQ = header >> 10;
	        printf("SEQ is: %lu\n",SEQ);

                data=data+mcnt;
                //fwrite(data,FRAME,1,sfp);
        }
        data=data-FRAME*SIZE;
        fwrite(data,FRAME*SIZE,1,sfp);
        free(data);
        fclose(sfp);

        int finish=time((time_t*)NULL);
        int Total_time=finish-start;
        printf("time consume is %d seconds\n",Total_time);
}


