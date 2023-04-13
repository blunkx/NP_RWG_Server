#include "np_simple.h"

inline void check_arg(int argc, char *const argv[], int &server_port);
inline void init_server(const int server_port, int &socket_fd, sockaddr_in &server_info);
inline void init_client(sockaddr_in &client_info);
inline void print_login_msg();
inline void print_lost_cnt_msg();

int main(int argc, char *const argv[])
{
    int server_port = 0;
    check_arg(argc, argv, server_port);

    sockaddr_in server_info, client_info;
    socklen_t client_sock_len = sizeof(client_info);
    int socket_fd = 0,
        client_fd = 0;

    init_server(server_port, socket_fd, server_info);
    init_client(client_info);

    int status;
    while (true)
    {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_info, &client_sock_len);
        print_login_msg();
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
            print_lost_cnt_msg();
        }
    }
    return 0;
}

inline void check_arg(int argc, char *const argv[], int &server_port)
{
    switch (argc)
    {
    case 1:
        server_port = 8080;
        break;
    case 2:
        try
        {
            server_port = std::stoi(argv[1]);
        }
        catch (std::exception &e)
        {
            std::cerr << "fail to tranfrom argument to int\n";
            exit(EXIT_FAILURE);
        }
        break;
    default:
        std::cerr << "unvalid argument number for np_simple\n";
        exit(EXIT_FAILURE);
    }
}

inline void init_server(const int server_port, int &socket_fd, sockaddr_in &server_info)
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        std::cerr << "fail to create socket\n";
        exit(EXIT_FAILURE);
    }
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
}

inline void init_client(sockaddr_in &client_info)
{
    bzero(&client_info, sizeof(client_info));
}

inline void print_login_msg()
{
    std::cout << "|---------------------------------------------------------|\n"
              << "|                        New user login                   |\n"
              << "|---------------------------------------------------------|\n\n";
}

inline void print_lost_cnt_msg()
{
    std::cout << "|-------------------User lost connection------------------|\n\n";
}