#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int getMySocket(){
    return 1;
}

struct msg {
    char *buffer;
    char code;
};

const size_t MAXDATASIZE = 100;

int main(int argc, char **argv) {
    int mySocket, numbytes; 
    struct msg MSG; 
    mySocket = getMySocket(); 
    numbytes=recv(mySocket,&(MSG.buffer), MAXDATASIZE, 0); 
    if(MSG.code==7){ 
        printf("Begin");    
        numbytes=recv(mySocket, &(MSG.buffer),  MAXDATASIZE, 0); 
    } 
    else { 
        printf("code: %c\n",MSG.code);                
        numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
    } 
    
    return 0;
}