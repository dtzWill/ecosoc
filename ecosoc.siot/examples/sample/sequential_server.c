#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

const size_t MAXDATASIZE = 100;

int main(int argc, char **argv) {
    char buffer[5];

    recv(1, &buffer, MAXDATASIZE, 0);
    recv(2, &buffer, MAXDATASIZE, 0);
    recv(3, &buffer, MAXDATASIZE, 0);
    recv(3, &buffer, MAXDATASIZE, 0);

    return 0;
}
