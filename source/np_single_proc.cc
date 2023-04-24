#include "np_single_proc.h"

inline void check_arg(int argc, char *const argv[], int &server_port);
inline void init_server(const int server_port, int &socket_fd, sockaddr_in &server_info);
inline void init_client(sockaddr_in &client_info);
inline void print_login_msg();
inline void print_lost_cnt_msg();
inline void init_fd_set();
size_t give_available_id(std::vector<user_info> user_info_arr);
inline void show_user_input(std ::string input, const std::vector<user_info> user_info_arr, size_t id);

int main(int argc, char *const argv[])
{
    int server_port = 0;
    check_arg(argc, argv, server_port);

    sockaddr_in server_info, new_con_info;
    socklen_t new_con_sock_len = sizeof(new_con_info);
    std::vector<user_info> user_info_arr;
    int socket_fd = 0;
    int new_con_fd = 0;

    int good_fd_num = 0;

    fd_set all_fds;
    FD_ZERO(&all_fds);

    init_server(server_port, socket_fd, server_info);
    init_client(new_con_info);
    FD_SET(socket_fd, &all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    init_env(); // set all env to bin:.
    while (true)
    {
        fd_set temp_fds;
        FD_ZERO(&temp_fds);
        temp_fds = all_fds;
        // after select, temp_fds includes all ready fd from all_fds
        if (user_info_arr.empty())
            good_fd_num = select(socket_fd + 1, &temp_fds, NULL, NULL, NULL);
        else
            good_fd_num = select(user_info_arr.back().fd + 1, &temp_fds, NULL, NULL, NULL);
        if (good_fd_num == -1)
        {
            std::cerr << "fail to selct\n";
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(STDIN_FILENO, &temp_fds))
        {
            char buffer[15001] = {0};
            ssize_t read_len = read(STDIN_FILENO, buffer, sizeof(buffer));
            std::string temp(buffer);
            if (read_len == -1)
            {
                std::cerr << "stdin read error\n";
                exit(EXIT_FAILURE);
            }
            else if (temp.find("exit") != std::string::npos)
            {
                close(socket_fd);
                exit(EXIT_SUCCESS);
            }
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
            user_info temp(new_con_info, new_con_fd, give_available_id(user_info_arr));
            user_info_arr.push_back(temp);
            broadcast(user_info_arr, LOG_IN, user_info_arr.size() - 1, "");
            if (good_fd_num == 1) // only socket fd works, back to select and wait for new connection
                continue;
        }
        // poll
        for (size_t i = 0; i < user_info_arr.size(); i++)
        {
            if (!user_info_arr[i].is_closed && FD_ISSET(user_info_arr[i].fd, &temp_fds))
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
                    broadcast(user_info_arr, LOG_OUT, i, "");
                    close(user_info_arr[i].fd);
                    FD_CLR(user_info_arr[i].fd, &all_fds);
                    print_lost_cnt_msg();
                }
                else
                {
                    // init_user_env();
                    std::map<std::string, std::string>::iterator it;
                    for (it = user_info_arr[i].env_var.begin(); it != user_info_arr[i].env_var.end(); it++)
                    {
                        setenv(it->first.c_str(), it->second.c_str(), true);
                    }
                    int stdout_copy = dup(STDOUT_FILENO);
                    int stderr_copy = dup(STDERR_FILENO);
                    std::string input(buffer);
                    show_user_input(input, user_info_arr, i);
                    dup2(user_info_arr[i].fd, STDOUT_FILENO);
                    dup2(user_info_arr[i].fd, STDERR_FILENO);
                    parser(input, user_info_arr, i);
                    if (!user_info_arr[i].is_closed)
                        std::cout << "% " << std::flush;
                    dup2(stdout_copy, STDOUT_FILENO);
                    dup2(stderr_copy, STDERR_FILENO);
                    close(stdout_copy);
                    close(stderr_copy);
                    std::cout << std::flush;
                    if (user_info_arr[i].is_closed)
                    {
                        shutdown(user_info_arr[i].fd, SHUT_RDWR);
                        FD_CLR(user_info_arr[i].fd, &all_fds);
                        print_lost_cnt_msg();
                    }
                }
            }
        }
        // remove all disconnected user from vector
        user_info_arr.erase(
            remove_if(
                user_info_arr.begin(),
                user_info_arr.end(),
                [](user_info const &p)
                { return p.is_closed; }),
            user_info_arr.end());
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
    int one = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
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
    std::cout << "server listen on port:" << server_port << std::endl;
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

size_t give_available_id(std::vector<user_info> user_info_arr)
{
    if (user_info_arr.empty())
    {
        return 1;
    }
    for (size_t i = 1; i <= 30; i++)
    {
        bool is_id_existed = false;
        for (size_t j = 0; j < user_info_arr.size(); j++)
        {
            if (i == user_info_arr[j].id_num)
            {
                is_id_existed = true;
                break;
            }
        }
        if (!is_id_existed)
            return i;
    }
    return 0;
}

inline void show_user_input(std ::string input, const std::vector<user_info> user_info_arr, size_t id)
{
    std::cout << user_info_arr[id].id_num << " " << user_info_arr[id].name << " send " << input << std::endl;
}