#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int getMyServerSocket(){
    return 1;
}

struct msgServer {
  char* buffer;  
};

int main(int argc, char **argv) {
    int mySocket, code; 
    struct msgServer MSG; 
    mySocket = getMyServerSocket();
    
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0); 
    sleep(2);
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0); 
    scanf("%d",&code); 
    int i = 0;
    for(i=0; i<code; ++i){
        //Send code=8 size=7 buffer=begin    
        strcpy(MSG.buffer,"78begin"); 
        send(mySocket, MSG.buffer, 7, 0);
        sleep(5);
        send(mySocket, MSG.buffer, 7, 0);
        sleep(5);
        send(mySocket, MSG.buffer, 7, 0);
    }
    scanf("%s",MSG.buffer); 
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0); 
    sleep(2);
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0); 
    sleep(2);
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
    sleep(2);
    send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
    
    return 0;
}
