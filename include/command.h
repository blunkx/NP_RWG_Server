
#ifndef COMMAND_H_
#define COMMAND_H_

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

using namespace std;

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
    vector<string> cmd;
    string which_type();
};

void init_env();
void print_env(const char *const para);

inline void init_pipe(int *fd);
inline void reduce_num_pipes(vector<command> &number_pipes, int last);
inline void init_temp_fd(vector<int *> &temp_fd_arr, size_t s);
void collect_num_pipe_output(vector<command> &cmds, vector<int *> &temp_fd_arr, size_t &temp_id, size_t i);
inline void close_unused_pipe_in_child(vector<command> &cmds, size_t i);
inline void close_pipe(vector<command> &cmds);
inline void close_temp_pipe(vector<int *> &temp_fd_arr);
inline void reduce_num_by_nl(vector<command> &cmds);
char **vector_to_c_str_arr(vector<string> cmd);

void exe_command(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_f_red(int stdout_copy, vector<command> &cmds, int i, int temp_fd[]);
void exe_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_bin(vector<command> &cmds);

void print_cmds(vector<command> cmds);

#endif
