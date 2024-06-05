#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

void *handleRequest(void *arg){
    int newsock = *(int*)arg;
    free(arg);
    char buff[1001];
    int mlen = recv(newsock, buff, sizeof(buff) - 1, 0);
    if(mlen > 0){
        buff[mlen] = '\0';
        printf("Received: %s\n", buff);
    }
    close(newsock);
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("ERROR: Must input TCP Port\n");
        return 1;
    }

    int portNum = atoi(argv[1]);
    if(portNum < 1024 || portNum > 65535){
        printf("ERROR: Port number must be between 1024 and 65535\n");
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0){
        perror("ERROR opening socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNum);

    int bindServer = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if(bindServer < 0){
        perror("ERROR on binding");
        return 1;
    }

    while(1){
        listen(serverSocket, 100);
        int *new_socket = malloc(sizeof(int));
        if (!new_socket) {
            perror("ERROR allocating memory");
            continue;
        }
        *new_socket = accept(serverSocket, NULL, NULL);
        if (*new_socket < 0) {
            perror("ERROR on accept");
            free(new_socket);
            continue;
        }
        pthread_t ntid;
        int new_thread = pthread_create(&ntid, NULL, handleRequest, new_socket);
        if(new_thread != 0){
            perror("ERROR creating thread");
            free(new_socket);
        }else{
            pthread_detach(ntid);
        }
    }
    close(serverSocket);
    return 0;
}
