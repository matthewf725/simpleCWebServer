#include "httpd.h"

int main(int argc, char *argv[]) {
    if(argc != 2){
        printf("ERROR: Must input TCP Port");
        return 1;
    }

    int portNum = argv[1];
    if(portNum < 1024 || portNum > 65535){
        printf("ERROR: Port number must be between 1024 and 65535");
        return 1;
    }


    return 0;
}