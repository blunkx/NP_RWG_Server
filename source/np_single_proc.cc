#include "np_single_proc.h"

inline void check_arg(int argc, char *const argv[], int &server_port);
inline void init_server(const int server_port, int &socket_fd, sockaddr_in &server_info);
inline void init_client(sockaddr_in &client_info);
inline void print_login_msg();
inline void print_lost_cnt_msg();
inline void init_fd_set();
void broadcast(const std::vector<user_info> &user_info_arr, BROADCAST_TYPE_E br_type, size_t log_out_id);

int main(int argc, char *const argv[])
{
    int server_port = 0;
    check_arg(argc, argv, server_port);

    sockaddr_in server_info, new_con_info;
    socklen_t new_con_sock_len = sizeof(new_con_info);
    std::vector<user_info> user_info_arr;
    int socket_fd = 0;
    int new_con_fd = 0;

    int next_available_fd = 0;

    fd_set all_fds;
    FD_ZERO(&all_fds);

    init_server(server_port, socket_fd, server_info);
    init_client(new_con_info);
    FD_SET(socket_fd, &all_fds);

    int status;
    while (true)
    {
        fd_set temp_fds;
        FD_ZERO(&temp_fds);
        temp_fds = all_fds;
        if (user_info_arr.empty())
            next_available_fd = select(socket_fd + 1, &temp_fds, NULL, NULL, NULL);
        else
            next_available_fd = select(user_info_arr.back().fd + 1, &temp_fds, NULL, NULL, NULL);
        if (next_available_fd == -1)
        {
            std::cerr << "fail to selct\n";
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(socket_fd, &temp_fds))
        {
            new_con_fd = accept(socket_fd, (struct sockaddr *)&new_con_info, &new_con_sock_len);
            if (new_con_fd == -1)
            {
                std::cerr << "New connect fd error\n";
                exit(EXIT_FAILURE);
            }
            print_login_msg();
            FD_SET(new_con_fd, &all_fds);
            user_info temp(new_con_info, new_con_fd);
            user_info_arr.push_back(temp);
            broadcast(user_info_arr, LOG_IN, 0);
            if (next_available_fd - 1 == 0) // only socket fd works, back to select and wait for new connection
                continue;
        }

        // poll
        for (size_t i = 0; i < user_info_arr.size(); i++)
        {
            if (FD_ISSET(user_info_arr[i].fd, &temp_fds))
            {
                char buffer[15001] = {0};
                ssize_t read_len = read(user_info_arr[i].fd, buffer, sizeof(buffer));
                if (read_len == -1)
                {
                    std::cerr << "client fd read error\n";
                    exit(EXIT_FAILURE);
                }
                else if (read_len == 0)
                {
                    user_info_arr[i].is_closed = true;
                    broadcast(user_info_arr, LOG_OUT, i);
                    close(user_info_arr[i].fd);
                    FD_CLR(user_info_arr[i].fd, &all_fds);
                    print_lost_cnt_msg();
                }
                else
                {

                    int stdout_copy = dup(STDOUT_FILENO);
                    int stderr_copy = dup(STDERR_FILENO);
                    dup2(user_info_arr[i].fd, STDOUT_FILENO);
                    dup2(user_info_arr[i].fd, STDERR_FILENO);
                    std::string input(buffer);
                    std::cout << input << std::flush;
                    std::cout << "%" << std::flush;
                    //  exe_shell();
                    dup2(stdout_copy, STDOUT_FILENO);
                    dup2(stderr_copy, STDERR_FILENO);
                    close(stdout_copy);
                    close(stderr_copy);
                    std::cout << std::flush;
                }
            }
        }
    }
    close(socket_fd);
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
    if (listen(socket_fd, 64) < 0)
    {
        std::cerr << "Fail to listen\n";
        exit(EXIT_FAILURE);
    }
}

inline void init_client(sockaddr_in &client_info)
{
    bzero(&client_info, sizeof(client_info));
}

inline void print_login_msg()
{
    std::cout
        << "|---------------------------------------------------------|\n"
        << "|                        New user login                   |\n"
        << "|---------------------------------------------------------|\n\n";
}

inline void print_lost_cnt_msg()
{
    std::cout << "|-------------------User lost connection------------------|\n\n";
}

void broadcast(const std::vector<user_info> &user_info_arr, BROADCAST_TYPE_E br_type, size_t log_out_id)
{

    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        int stdout_copy = dup(STDOUT_FILENO);
        dup2(user_info_arr[i].fd, STDOUT_FILENO);
        switch (br_type)
        {
        case LOG_IN:
            if (i == user_info_arr.size() - 1)
            {
                std::cout << "****************************************\n"
                          << "** Welcome to the information server. **\n"
                          << "****************************************" << std::flush;
            }
            std::cout << "\n*** User \'("
                      << user_info_arr[i].name
                      << ")\' entered from "
                      << inet_ntoa(user_info_arr[i].sock_addr_info.sin_addr)
                      << ":" << ntohs(user_info_arr[i].sock_addr_info.sin_port)
                      << ". ***" << std::endl;
            std::cout << "%" << std::flush;
            break;
        case LOG_OUT:
            if (!user_info_arr[i].is_closed)
            {
                std::cout << "\n*** User \'<" << user_info_arr[log_out_id].name << ">\' left. ***" << std::endl;
                std::cout << "%" << std::flush;
            }
            break;
        case USER_PIPE:
            break;

        default:
            break;
        }
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
    }
}