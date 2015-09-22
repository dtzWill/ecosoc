#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int main(int argc, char **argv) {
    int sockfd;
    int BUFFERSIZE = 100;
    char infobuf1[BUFFERSIZE];
    char infobuf2[BUFFERSIZE];
    char infobuf3[BUFFERSIZE];
    
sleep(1);    
    send(sockfd, infobuf1, BUFFERSIZE-1, 0);
sleep(1);    
    send(sockfd, infobuf2, BUFFERSIZE-1, 0);
sleep(1);    
    for(int i=0; i<4; ++i)    
        send(sockfd, infobuf3, BUFFERSIZE-1, 0);
    
    return 0;
}
