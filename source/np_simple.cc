#include "np_simple.h"

int main(int argc, char *const argv[])
{
    long server_port = -1;
    server_port = 8080;
    // if (argc != 2)
    // {
    //     cerr << "unvalid argument for np_simple\n";
    //     exit(EXIT_FAILURE);
    // }
    // else
    // {
    //     server_port = stol(argv[1]);
    // }
    // int socket(int domain, int type, int protocol);
    // domain => AF_UNIX/AF_LOCAL : local file system   AF_INET : ipv4  AF_INET6 : ipv6
    // type => SOCK_STREAM :    TCP SOCK_DGRAM : UDP
    // protocol => 0 :
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        cerr << "fail to create socket\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));  // init server to 0
    server_info.sin_family = PF_INET;          // ipv4
    server_info.sin_addr.s_addr = INADDR_ANY;  // allow all ip to connect
    server_info.sin_port = htonl(server_port); // listening port
    bind(socket_fd, (struct sockaddr *)&server_info, sizeof(server_info));
    // if (< 0)
    // {
    //     cerr << "Fail to bind\n";
    //     exit(EXIT_FAILURE);
    // }
    // int listen(int sockfd, int backlog); backlog => length if queue
    listen(socket_fd, 5);
    struct sockaddr_in client_info;
    int client_fd = 0;
    bzero(&client_info, sizeof(client_info));
    while (true)
    {
        socklen_t client_sock_len = sizeof(client_info);
        client_fd = accept(socket_fd, (struct sockaddr *)&server_info, &client_sock_len);
        dup2(client_fd, STDIN_FILENO);
        dup2(client_fd, STDOUT_FILENO);
        dup2(client_fd, STDERR_FILENO);
        exe_shell();
    }

    // exe_shell();
    return 0;
}
