
#ifndef _COMMAND_H_
#define _COMMAND_H_

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

typedef enum PIPE_TYPE_T
{
    NO_PIPE,
    PIPE,
    ERR_PIPE,
    F_RED_PIPE,
    NUM_PIPE,
    ERR_NUM_PIPE,
    WRITE_USER_PIPE,
    READ_USER_PIPE
} PIPE_TYPE_E;

typedef enum BROADCAST_TYPE_T
{
    LOG_IN,
    LOG_OUT,
    WR_USER_PIPE_BR,
    RD_USER_PIPE_BR,
    YELL_BR,
    CHANGE_NAME
} BROADCAST_TYPE_E;

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
    user_info(sockaddr_in input_info, int input_fd, size_t input_id)
    {
        sock_addr_info = input_info;
        fd = input_fd;
        id_num = input_id;
        env_var["PATH"] = "bin:.";
    }
    size_t id_num = 0;
    std::string name = "(no name)";
    sockaddr_in sock_addr_info;
    int fd = -1;
    bool is_closed = false;
    std::map<std::string, std::string> env_var;
    std::string recv_input;
    std::vector<command> cmds;
    std::map<size_t, int *> user_pipe;
};

void init_env();
void print_env(const char *const para);
void print_users(std::vector<user_info> user_info_arr, const size_t id);
void tell_to_other(const std::vector<user_info> &user_info_arr, const size_t sender_id, const size_t recv_id, std::string msg);
void change_name(std::vector<user_info> &user_info_arr, const size_t id, std::string input_name);
void broadcast(const std::vector<user_info> &user_info_arr, BROADCAST_TYPE_E br_type, size_t log_out_id, std::string msg);

void exe_bin(std::vector<command> &cmds);
void exe_bin(std::vector<user_info> &user_info_arr, size_t id);

void print_cmds(std::vector<command> cmds);

#endif
