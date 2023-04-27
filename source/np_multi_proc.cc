#include "np_multi_proc.h"

inline void check_arg(int argc, char *const argv[], int &server_port);
inline void init_server(const int server_port, int &socket_fd, sockaddr_in &server_info);
inline void init_client(sockaddr_in &client_info);
inline void print_login_msg();
inline void print_login_msg_to_client(int socket_fd, const user_info_shm_ver login_cli);
inline void print_lost_cnt_msg();
inline void init_user_info(user_info_shm_ver *user_info_arr);
size_t give_available_id(user_info_shm_ver *user_info_arr);
inline void show_user_input(std ::string input, const std::vector<user_info> user_info_arr, size_t id);

int main(int argc, char *const argv[])
{
    int server_port = 0;
    check_arg(argc, argv, server_port);

    sockaddr_in server_info, new_con_info;
    socklen_t new_con_sock_len = sizeof(new_con_info);
    int socket_fd = 0;
    int new_con_fd = 0;
    init_server(server_port, socket_fd, server_info);
    init_client(new_con_info);
    init_env(); // set all env to bin:.

    user_info_shm_ver user_info_arr[30];
    int shm_fd = shm_open("user_info_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        std::cerr << "shared memory error\n";
    }
    ftruncate(shm_fd, sizeof(user_info_arr));
    user_info_shm_ver *shm_user_info_arr = (user_info_shm_ver *)mmap(NULL, sizeof(user_info_arr), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memcpy(shm_user_info_arr, &user_info_arr, sizeof(user_info_arr));
    init_user_info(shm_user_info_arr);

    size_t id = 0;
    while (true)
    {
        new_con_fd = accept(socket_fd, (struct sockaddr *)&new_con_info, &new_con_sock_len);
        if (new_con_fd == -1)
        {
            std::cerr << "New connect fd error\n";
            exit(EXIT_FAILURE);
        }
        else
        {
            id = give_available_id(shm_user_info_arr);
            print_login_msg();
            shm_user_info_arr[id].id_num = id + 1;
            shm_user_info_arr[id].fd = new_con_fd;
            shm_user_info_arr[id].sock_addr_info = new_con_info;
            print_login_msg_to_client(new_con_fd, shm_user_info_arr[id]);
            broadcast(shm_user_info_arr, LOG_IN, id, "");
        }
        pid_t pid = fork();
        if (pid == -1)
        {
            std::cerr << "fork error!\n";
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            close(socket_fd);
            dup2(new_con_fd, STDIN_FILENO);
            dup2(new_con_fd, STDOUT_FILENO);
            dup2(new_con_fd, STDERR_FILENO);
            exe_shell(shm_user_info_arr, id);
        }
        else
        {
            close(new_con_fd);
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

inline void print_login_msg_to_client(int socket_fd, const user_info_shm_ver login_cli)
{
    int stdout_copy = dup(STDOUT_FILENO);
    dup2(socket_fd, STDOUT_FILENO);
    std::cout << "****************************************\n"
              << "** Welcome to the information server. **\n"
              << "****************************************" << std::endl;
    std::cout << "*** User \'"
              << login_cli.name << "\' entered from "
              << inet_ntoa(login_cli.sock_addr_info.sin_addr) << ":"
              << ntohs(login_cli.sock_addr_info.sin_port) << ". ***" << std::endl;
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);
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

inline void init_user_info(user_info_shm_ver *user_info_arr)
{
    for (size_t i = 0; i < 30; i++)
    {
        strcpy(user_info_arr[i].name, "(no name)");
    }
}

size_t give_available_id(user_info_shm_ver *user_info_arr)
{
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num == 0)
            return i;
    }
    return 0;
}

inline void show_user_input(std ::string input, const std::vector<user_info> user_info_arr, size_t id)
{
    std::cout << user_info_arr[id].id_num << " " << user_info_arr[id].name << " send " << input << std::endl;
}