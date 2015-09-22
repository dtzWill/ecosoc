#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

int main(int argc, char **argv) {
    char buffer[10];

    scanf ("%10s",buffer);
    send(1, buffer, 10, 0);
    send(2, buffer, strlen(buffer), 0);
    scanf("%s",buffer);
    send(3, buffer, strlen(buffer), 0);
    send(4, buffer, 10, 0);

    return 0;
}
