
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
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

typedef enum PIPE_TYPE_T
{
    NO_PIPE,
    PIPE,
    ERR_PIPE,
    F_RED_PIPE,
    NUM_PIPE,
    ERR_NUM_PIPE,

} PIPE_TYPE_E;

class command
{
private:
public:
    PIPE_TYPE_E pipe_type;
    int pipe_num = 0;
    bool is_exe = false;
    bool is_piped = false;
    int fd[2];
    std::vector<std::string> cmd;
    std::string which_type();
};

class user_info
{
private:
public:
    user_info(sockaddr_in input_info, int input_fd)
    {
        sock_addr_info = input_info;
        fd = input_fd;
    }
    sockaddr_in sock_addr_info;
    int fd = -1;
    std::string name = "no name";
    std::vector<command> cmds;
    bool is_closed = false;
};

void init_env();
void print_env(const char *const para);

inline void init_pipe(int *fd);
inline void reduce_num_pipes(std::vector<command> &number_pipes, int last);
inline void init_temp_fd(std::vector<int *> &temp_fd_arr, size_t s);
void collect_num_pipe_output(std::vector<command> &cmds, std::vector<int *> &temp_fd_arr, size_t &temp_id, size_t i);
inline void close_unused_pipe_in_child(std::vector<command> &cmds, size_t i);
inline void close_pipe(std::vector<command> &cmds);
inline void close_temp_pipe(std::vector<int *> &temp_fd_arr);
inline void reduce_num_by_nl(std::vector<command> &cmds);
char **vector_to_c_str_arr(std::vector<std::string> cmd);

void exe_command(int stdout_copy, std::vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_pipe(int stdout_copy, std::vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_pipe(int stdout_copy, std::vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_f_red(int stdout_copy, std::vector<command> &cmds, int i, int temp_fd[]);
void exe_num_pipe(int stdout_copy, std::vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_num_pipe(int stdout_copy, std::vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_bin(std::vector<command> &cmds);

void print_cmds(std::vector<command> cmds);

#endif
