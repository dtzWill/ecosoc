#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

int getMySocket2();

int randomize() {
    rand();
	if(rand() > 0) {
		return 1;
    }

    return -1;
}

void doSomething() {
	switch(rand()){
	case 2:
		randomize();
		break;
	case 3:
		randomize();
        rand();
		break;
	}
}

int getMySocket(){
	if(randomize()>1){
		doSomething();
		return 2;
	}
	return 1;
}

struct msg {
	char *buffer;
	char code;
};

const size_t MAXDATASIZE = 100;

int main() {
	int mySocket, numbytes;
	struct msg MSG;
	mySocket = getMySocket();
	numbytes=recv(mySocket,&(MSG.buffer), MAXDATASIZE, 0);
	numbytes=recv(mySocket,&(MSG.buffer), MAXDATASIZE, 0);
	if(MSG.code==7){
		printf("Begin");
		numbytes=recv(mySocket, &(MSG.buffer),  MAXDATASIZE, 0);
	}
	else {
		printf("code: %c\n",MSG.code);
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
	}
	mySocket = getMySocket2();
	numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);

	switch (MSG.code) {
	case 11:
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		break;
	case 12:
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		break;
	default:
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);
		break;
	}

    mySocket = getMySocket();
	numbytes=recv(mySocket, &(MSG.buffer), MAXDATASIZE, 0);

	return 0;
}

int getMySocket2(){
	if(randomize()>3){
		return 5;
	}
	return 7;
}
