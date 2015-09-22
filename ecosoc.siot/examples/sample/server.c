#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

void receber(){
        int sockfd;
    int BUFFERSIZE = 100;
    char infobuf[BUFFERSIZE];

    recv(sockfd,infobuf,BUFFERSIZE-1,0);//records from mem
}

int tttmain(int argc, char **argv) {
    int sockfd;
    int BUFFERSIZE = 50;
    char infobuf1[BUFFERSIZE];
    char infobuf2[BUFFERSIZE];
    char infobuf3[BUFFERSIZE];
  
    receber();
    recv(sockfd,infobuf1,BUFFERSIZE-1,0);//records from mem
    if(argc > 1) {
      recv(sockfd,infobuf2,BUFFERSIZE-1,0);//records from mem
    } else {
        recv(sockfd,infobuf3,BUFFERSIZE-1,0);//records from mem
    }
  return 0;
}



