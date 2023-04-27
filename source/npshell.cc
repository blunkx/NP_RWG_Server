#include "npshell.h"

void exe_shell()
{
    std::string input;
    std::vector<command> cmds;
    while (true)
    {
        std::cout << "% ";
        if (!std::getline(std::cin, input).eof())
        {
            parser(input, cmds);
        }
        else
        {
            parser(input, cmds);
            break;
        }
    }
}

void exe_shell(user_info_shm_ver *user_info_arr, size_t id)
{

    std::vector<command> cmds;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    std::cout << "% " << std::flush;
    while (true)
    {
        if (strlen(user_info_arr[id].broadcast_msg) != 0)
        {
            std::cout << user_info_arr[id].broadcast_msg << std::flush;
            memset(user_info_arr[id].broadcast_msg, 0, sizeof(user_info_arr[id].broadcast_msg));
        }
        char buffer[15001] = {0};
        ssize_t read_len = read(STDIN_FILENO, buffer, sizeof(buffer));
        std::string input(buffer);
        if (read_len > 0)
        {
            parser(input, cmds, user_info_arr, id);
            usleep(1000);
            if (strlen(user_info_arr[id].broadcast_msg) != 0)
            {
                std::cout << user_info_arr[id].broadcast_msg << std::flush;
                memset(user_info_arr[id].broadcast_msg, 0, sizeof(user_info_arr[id].broadcast_msg));
            }
            std::cout << "% " << std::flush;
        }
        usleep(1000);
    }
}