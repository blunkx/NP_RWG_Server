#include "np_simple.h"

int main(int argc, char *const argv[])
{
    int server_port = 0;
    switch (argc)
    {
    case 1:
        server_port = 8700;
        break;
    case 2:
        server_port = std::stoi(argv[1]);
        break;
    default:
        std::cerr << "unvalid argument for np_simple\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_info, client_info;
    int socket_fd = 0, client_fd = 0;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        std::cerr << "fail to create socket\n";
        exit(EXIT_FAILURE);
    }
    bzero(&client_info, sizeof(client_info));
    socklen_t client_sock_len = sizeof(client_info);
    bzero(&server_info, sizeof(server_info));  // init server to 0
    server_info.sin_family = PF_INET;          // ipv4
    server_info.sin_addr.s_addr = INADDR_ANY;  // allow all ip to connect
    server_info.sin_port = htons(server_port); // listening port
    if (bind(socket_fd, (struct sockaddr *)&server_info, sizeof(server_info)) < 0)
    {
        std::cerr << "Fail to bind\n";
        exit(EXIT_FAILURE);
    }
    listen(socket_fd, 5);
    int status;
    while (true)
    {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_info, &client_sock_len);

        std::cout << "|---------------------------------------------------------|\n"
                  << "|                        New user login                   |\n"
                  << "|---------------------------------------------------------|\n\n";
        pid_t pid;
        pid = fork();
        if (pid == -1)
        {
            std::cerr << "fork error!\n";
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            dup2(client_fd, STDIN_FILENO);
            dup2(client_fd, STDOUT_FILENO);
            dup2(client_fd, STDERR_FILENO);
            exe_shell();
        }
        else
        {
            close(client_fd);
            waitpid(pid, &status, 0);
            std::cout << "|-------------------User lost connection------------------|\n\n";
        }
    }
    return 0;
}
