#include "httpd.h"

#define BUFFER_SIZE 4096
#define SMALL_BUFFER_SIZE 1001
#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 100

//Sends an HTTP response with the given status, content type, and body
void send_response(int socket, char *status, const char *content_type, const char *body, int body_length) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", status, content_type, body_length);
    send(socket, response, strlen(response), 0);
    if (body && body_length > 0) {
        send(socket, body, body_length, 0);
    }
}

//Checks if the request path includes a delay and returns the delay value
int check_for_delay(char* path){
    if (strncmp(path, "/delay/", strlen("/delay/")) == 0) {
        const char *num_str = path + strlen("/delay/");
        while (*num_str) {
            if (!isdigit((unsigned char)*num_str)) {
                return -1;
            }
            num_str++;
        }
        return atoi(path + strlen("/delay/"));
    }
    return -1;
}

//Handles GET or HEAD requests by serving the requested file or handling delay
void handle_get_or_head_request(int socket, char* path, int get) {
    int delay = check_for_delay(path);
    if (delay != -1) {
        sleep(delay);
        send_response(socket, "200 OK", "text/html", NULL, delay);
    } else if (strstr(path, "..") != NULL || strchr(path, '~') != NULL) {
        char errMsg[] = "HTTP/1.1 404 Not Found\r\n";
        send_response(socket, "404 Not Found", "text/html", errMsg, strlen(errMsg));
    } else {
        if (strncmp(path, "./", 2) == 0) {
            path = path + 2;
        } else if (strncmp(path, "/", 1) == 0) {
            path = path + 1;
        }
        FILE* fp = fopen(path, "r");
        if (fp == NULL) {
            char errMsg[] = "HTTP/1.1 404 Not Found\r\n";
            send_response(socket, "404 Not Found", "text/html", errMsg, strlen(errMsg));
        } else {
            struct stat st;
            stat(path, &st);
            if (get) {
                char file_contents[st.st_size + 1];
                fread(file_contents, 1, st.st_size, fp);
                file_contents[st.st_size] = '\0';
                send_response(socket, "200 OK", "text/html", file_contents, st.st_size);
            } else {
                fclose(fp);
                send_response(socket, "200 OK", "text/html", NULL, st.st_size);
            }
        }
    }
}

//Handles incoming HTTP requests and delegates to appropriate handler functions
void *handle_request(void *arg) {
    int newsock = *(int *)arg;
    free(arg);
    char buff[SMALL_BUFFER_SIZE] = {0};
    int mlen = recv(newsock, buff, sizeof(buff) - 1, 0);
    if (mlen > 0) {
        char *method = strtok(buff, " ");
        char *path = strtok(NULL, " ");
        char *version = strtok(NULL, "\r\n");
        if (!method || !path || !version) {
            send(newsock, "HTTP/1.1 400 Bad Request\r\n", strlen("HTTP/1.1 400 Bad Request\r\n"), 0);
        } else if (strcmp(version, "HTTP/1.1") != 0) {
            send(newsock, "HTTP/1.1 400 Bad Request\r\n", strlen("HTTP/1.1 400 Bad Request\r\n"), 0);
        } else if (strcmp(method, "GET") == 0) {
            handle_get_or_head_request(newsock, path, 1);
        } else if (strcmp(method, "HEAD") == 0) {
            handle_get_or_head_request(newsock, path, 0);
        } else {
            char errMsg[] = "HTTP/1.1 501 Not Implemented\r\n";
            send_response(newsock, "501 Not Implemented", "text/html", errMsg, strlen(errMsg));
        }
    } else {
        send(newsock, "HTTP/1.1 400 Bad Request\r\n", strlen("HTTP/1.1 400 Bad Request\r\n"), 0);
    }
    close(newsock);
    return NULL;
}

//Main function to set up the server and handle incoming connections
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("ERROR: Must input TCP Port\n");
        return 1;
    }

    int portNum = atoi(argv[1]);
    if (portNum < 1024 || portNum > 65535) {
        printf("ERROR: Port number must be between 1024 and 65535\n");
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("ERROR opening socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNum);
    const int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    int bindServer = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindServer < 0) {
        perror("ERROR on binding");
        return 1;
    } else {
        printf("Server started on port %d\n", portNum);
    }

    while (1) {
        listen(serverSocket, LISTEN_BACKLOG);
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
        int new_thread = pthread_create(&ntid, NULL, handle_request, new_socket);
        if (new_thread != 0) {
            perror("ERROR creating thread");
            free(new_socket);
        } else {
            pthread_detach(ntid);
        }
    }
    close(serverSocket);
    return 0;
}
