#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

//GET /httpd.c HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n
void send_response(int socket, const char *status, const char *content_type, const char *body, int body_length) {
    char response[4096];
    snprintf(response, sizeof(response), "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", status, content_type, body_length);
    send(socket, response, strlen(response), 0);
    if (body && body_length > 0) {
        send(socket, body, body_length, 0);
    }
}


void *handleRequest(void *arg) {
    int newsock = *(int *)arg;
    free(arg);

    char buff[1001];
    int mlen = recv(newsock, buff, sizeof(buff) - 1, 0);
    if (mlen > 0) {
        buff[mlen] = '\0';
        char *method = strtok(buff, " ");
        char *path = strtok(NULL, " ");
        char *version = strtok(NULL, "\r");

        if (!method || !path || !version) {
            char errMsg[] = "HTTP/1.1 400 Bad Request  (malformed or unparseable HTTP request)\r\n";
            send_response(newsock, "400 Bad Request", "text/html", errMsg, strlen(errMsg));
        // } else if (strcmp(method, "GET") == 0) {
        //     handle_get_request(newsock, path);
        // } else if (strcmp(method, "HEAD") == 0) {
        //     handle_head_request(newsock, path);
        } else {
            char errMsg[] = "HTTP/1.1 501 Not Implemented  (request that uses an action other than HEAD or GET)\r\n";
            send_response(newsock, "501 Not Implemented", "text/html", errMsg, strlen(errMsg));
        }
    } else {
        char errMsg[] = "HTTP/1.1 400 Bad Request  (malformed or unparseable HTTP request)\r\n";
        send_response(newsock, "400 Bad Request", "text/html", errMsg, strlen(errMsg));
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
    } else {
        printf("Server started on port %d\n", portNum);
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
