#include <iostream>
#include <string>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct
{
    char key[100] = {0};
    char value[100] = {0};
} env_var_shm_ver;

typedef struct
{
    size_t send_to_id;
    char value[100] = {0};
} user_pipe_shm_ver;

typedef struct
{
    size_t id_num = 0;
    char name[100] = {0};
    sockaddr_in sock_addr_info;
    int fd = -1;
    bool is_closed = false;
    env_var_shm_ver env_var[20];
    char recv_input[1024] = {0};
    user_pipe_shm_ver user_pipe;
} user_info_shm_ver;

int main(int argc, char *const argv[])
{
    user_info_shm_ver user_info_arr[30];
    int shm_fd = shm_open("user_info_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        std::cerr << "shared memory error\n";
    }
    ftruncate(shm_fd, sizeof(user_info_arr));
    user_info_shm_ver *ptr = (user_info_shm_ver *)mmap(NULL, sizeof(user_info_arr), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memcpy(ptr, &user_info_arr, sizeof(user_info_arr));

    std::string n;
    size_t total_num = 0;
    pid_t pid;
    int status;

    while (std::cin >> n)
    {
        // write user info into shared memory
        memcpy(ptr[total_num].name, n.c_str(), sizeof(n.c_str()));
        total_num++;
        pid = fork();
        if (pid == -1)
        {
            std::cerr << "fork error!\n";
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            std::cout << "child" << std::endl;
            char temp[] = "child";
            memcpy(ptr[total_num - 1].name, temp, sizeof(temp));
            for (size_t i = 0; i < total_num; i++)
            {
                std::cout << "user" << i << ": " << ptr[i].name << std::endl;
            }
            std::cout << std::endl;
            exit(0);
        }
        else
        {
            waitpid(pid, &status, 0);
            for (size_t i = 0; i < total_num; i++)
            {
                std::cout << "user" << i << ": " << ptr[i].name << std::endl;
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
