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

int main() {
	int mySocket, code;
	struct msgServer MSG;
	mySocket = getMyServerSocket();

	send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
	sleep(2);
	send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
	scanf("%d",&code);
	if(code == 7){
		//Send code=8 size=7 buffer=begin
		strcpy(MSG.buffer,"78begin");
		send(mySocket, MSG.buffer, 7, 0);
		sleep(5);
		send(mySocket, MSG.buffer, 7, 0);
		if(code == 8){
			//Send code=8 size=7 buffer=begin
			strcpy(MSG.buffer,"78begin");
			send(mySocket, MSG.buffer, 7, 0);
		}
		else {
			scanf("%s",MSG.buffer);
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
		}

	}
	else {
		scanf("%s",MSG.buffer);
		send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
		sleep(5);
		send(mySocket, MSG.buffer, 7, 0);
		sleep(5);
		send(mySocket, MSG.buffer, 7, 0);
	}
	if(code == 8){
		//Send code=8 size=7 buffer=begin
		strcpy(MSG.buffer,"78begin");
		send(mySocket, MSG.buffer, 7, 0);
	}
	else {
		scanf("%s",MSG.buffer);
		send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
		int i = 0;
		for(i=0; i<code; ++i){
			//Send code=8 size=7 buffer=begin
			strcpy(MSG.buffer,"78begin");
			send(mySocket, MSG.buffer, 7, 0);
			send(mySocket, MSG.buffer, 7, 0);
		}
		while(i<code){
			send(mySocket, MSG.buffer, 7, 0);
			send(mySocket, MSG.buffer, 7, 0);
			send(mySocket, MSG.buffer, 7, 0);
			++i;
		}
	}
	scanf("%s",MSG.buffer);
	send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
	sleep(2);
	switch (code) {
		case 11:
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			break;
		case 12:
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			break;
		default:
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);
			break;
	}
	send(mySocket, MSG.buffer, strlen(MSG.buffer), 0);

	return 0;
}
