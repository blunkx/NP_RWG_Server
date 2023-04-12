#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// int socket(int domain, int type, int protocol);
// domain => AF_UNIX/AF_LOCAL : local file system   AF_INET : ipv4  AF_INET6 : ipv6
// type => SOCK_STREAM :    TCP SOCK_DGRAM : UDP
// protocol => 0 :

// int listen(int sockfd, int backlog); backlog => length if queue

int main(int argc, char *argv[])
{
    // socket的建立
    char inputBuffer[256] = {};
    char message[] = {"Hi,this is server.\n"};
    int sockfd = 0, forClientSockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
    }

    // socket的連線
    struct sockaddr_in serverInfo, clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo, sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8700);
    bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
    listen(sockfd, 5);

    while (1)
    {
        forClientSockfd = accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen);
        send(forClientSockfd, message, sizeof(message), 0);
        recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);
        printf("Get:%s\n", inputBuffer);
    }
    return 0;
}