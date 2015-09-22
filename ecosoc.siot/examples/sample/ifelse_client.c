#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

//int main(int argc, char **argv) {
int main() {
    char buffer[10];

    scanf ("%10s",buffer);
    send(1, buffer, 10, 0);
    if(buffer[0]) {
        send(2, buffer, strlen(buffer), 0);
    }
    scanf("%s",buffer);
    if(buffer[1]) {
        send(3, buffer, strlen(buffer), 0);
    } else {
        send(4, buffer, strlen(buffer), 0);
    }
    send(5, buffer, 10, 0);

    return 0;
}
