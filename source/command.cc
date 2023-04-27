#include "command.h"

using namespace std;
// funciton only used in command.cc
inline void init_pipe(int *fd);
inline void reduce_num_pipes(vector<command> &number_pipes, int last);
inline void init_temp_fd(vector<int *> &temp_fd_arr, size_t s);
void collect_num_pipe_output(vector<command> &cmds, vector<int *> &temp_fd_arr, size_t &temp_id, size_t i);
inline void close_unused_pipe_in_child(vector<command> &cmds, size_t i);
inline void close_pipe(vector<command> &cmds);
inline void close_temp_pipe(vector<int *> &temp_fd_arr);
inline void close_wr_user_pipe(vector<user_info> &user_info_arr, int id, int i);
inline void close_rd_user_pipe(vector<user_info> &user_info_arr, int id, int i);
inline void close_wr_user_pipe(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i);
inline void close_rd_user_pipe(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i);

inline void reduce_num_by_nl(vector<command> &cmds);
inline int init_rd_user_pp(vector<user_info> &user_info_arr, int id, int cmd_i);
inline int init_wr_user_pp(vector<user_info> &user_info_arr, int id, int cmd_i);
inline int init_rd_user_pp(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i);
inline int init_wr_user_pp(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i);
inline bool is_rd_user_pp_exist(command cmd);
inline bool is_wr_user_pp_exist(command cmd);

char **vector_to_c_str_arr(vector<string> cmd);
void exe_command(int stdout_copy, vector<user_info> &user_info_arr, int id, int i, bool stop_pipe, int temp_fd[]);
void exe_command(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_command(int stdout_copy, vector<command> &cmds, user_info_shm_ver *user_info_arr, int id, int i, bool stop_pipe, int temp_fd[]);
void exe_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_f_red(int stdout_copy, vector<command> &cmds, int i, int temp_fd[]);
void exe_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);
void exe_err_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[]);

/*
 * multi proc
 */
void exe_bin(vector<command> &cmds, user_info_shm_ver *user_info_arr, size_t id)
{
    int status;
    // int stdin_copy = dup(STDIN_FILENO);
    int stdout_copy = dup(STDOUT_FILENO);
    vector<int *> temp_fd_arr;
    init_temp_fd(temp_fd_arr, 5); /*only support 5 number pipe in a line*/
    bool stop_pipe = true;
    pid_t last_pid = -1;
    pid_t pre_pid;
    size_t temp_id = 0;
    for (size_t i = 0; i < cmds.size(); i++)
    {
        if (!cmds[i].is_exe)
        {
            init_pipe(cmds[i].fd);
        }
    }
    for (size_t i = 0; i < cmds.size(); i++)
    {
        if (!cmds[i].is_exe)
        {
            collect_num_pipe_output(cmds, temp_fd_arr, temp_id, i);
            if ((cmds[i].pipe_type == NUM_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE))
                reduce_num_pipes(cmds, i);
            if (is_rd_user_pp_exist(cmds[i]))
            {
                if (init_rd_user_pp(user_info_arr, id, cmds, i) == -1)
                {
                    cmds[i].is_exe = true;
                    cmds[i].is_piped = true;
                    waitpid(pre_pid, &status, 0);
                    continue;
                }
            }
            if (is_wr_user_pp_exist(cmds[i]))
            {
                if (init_wr_user_pp(user_info_arr, id, cmds, i) == -1)
                {
                    cmds[i].is_exe = true;
                    cmds[i].is_piped = true;
                    waitpid(pre_pid, &status, 0);
                    continue;
                }
            }
            pid_t pid;
            pid = fork();
            if (pid == -1)
            {
                cerr << "fork error!\n";
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                //  child process
                close_unused_pipe_in_child(cmds, i);
                if (cmds[i].pipe_type == F_RED_PIPE)
                    exe_f_red(stdout_copy, cmds, i, temp_fd_arr[temp_id]);
                else
                    exe_command(stdout_copy, cmds, user_info_arr, id, i, stop_pipe, temp_fd_arr[temp_id]);
            }
            else
            {
                // parent process
                if (cmds[i].pipe_type == NO_PIPE || cmds[i].pipe_type == F_RED_PIPE)
                {
                    if (!is_wr_user_pp_exist(cmds[i]))
                    {
                        last_pid = pid;
                    }
                }
                if (cmds[i].pipe_type == NUM_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE)
                {
                    stop_pipe = true;
                }
                else
                {
                    stop_pipe = false;
                    cmds[i].is_piped = true;
                }
                cmds[i].is_exe = true;
                if (temp_id != 0)
                {
                    close(temp_fd_arr[temp_id][0]);
                    close(temp_fd_arr[temp_id][1]);
                }
                if (cmds[i].read_from != -1)
                {
                    close_rd_user_pipe(user_info_arr, id, cmds, i);
                }
                if (cmds[i].write_to != -1)
                {
                    close_wr_user_pipe(user_info_arr, id, cmds, i);
                }
                pre_pid = pid;
            }
        }
    }
    close_pipe(cmds);
    close_temp_pipe(temp_fd_arr);
    if (last_pid != -1)
        waitpid(last_pid, &status, 0);
    reduce_num_by_nl(cmds);
    cmds.erase(
        remove_if(
            cmds.begin(),
            cmds.end(),
            [](command const &p)
            { return p.is_piped; }),
        cmds.end());
    return;
}

/*
 * User pipe version
 */
void exe_bin(vector<user_info> &user_info_arr, size_t id)
{
    user_info *current_user = &(user_info_arr[id]);
    int status;
    int stdout_copy = dup(STDOUT_FILENO);
    vector<int *> temp_fd_arr;
    init_temp_fd(temp_fd_arr, 5); /*only support 5 number pipe in a line*/
    bool stop_pipe = true;
    pid_t last_pid = -1;
    size_t temp_id = 0;
    for (size_t i = 0; i < current_user->cmds.size(); i++)
    {
        if (!current_user->cmds[i].is_exe)
        {
            init_pipe(current_user->cmds[i].fd);
        }
    }
    pid_t pre_pid;
    for (size_t i = 0; i < current_user->cmds.size(); i++)
    {
        if (!current_user->cmds[i].is_exe)
        {
            collect_num_pipe_output(current_user->cmds, temp_fd_arr, temp_id, i);
            if (current_user->cmds[i].pipe_type == NUM_PIPE ||
                current_user->cmds[i].pipe_type == ERR_NUM_PIPE)
                reduce_num_pipes(current_user->cmds, i);
            if (is_rd_user_pp_exist(current_user->cmds[i]))
            {
                if (init_rd_user_pp(user_info_arr, id, i) == -1)
                {
                    current_user->cmds[i].is_exe = true;
                    current_user->cmds[i].is_piped = true;
                    waitpid(pre_pid, &status, 0);
                    continue;
                }
            }
            if (is_wr_user_pp_exist(current_user->cmds[i]))
            {
                if (init_wr_user_pp(user_info_arr, id, i) == -1)
                {
                    current_user->cmds[i].is_exe = true;
                    current_user->cmds[i].is_piped = true;
                    waitpid(pre_pid, &status, 0);
                    continue;
                }
            }
            pid_t pid;
            pid = fork();
            if (pid == -1)
            {
                cerr << "fork error!\n";
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                //  child process
                close_unused_pipe_in_child(current_user->cmds, i);
                // to do: dispatcher function
                if (current_user->cmds[i].pipe_type == F_RED_PIPE)
                    exe_f_red(stdout_copy, current_user->cmds, i, temp_fd_arr[temp_id]);
                else
                    exe_command(stdout_copy, user_info_arr, id, i, stop_pipe, temp_fd_arr[temp_id]);
            }
            else
            {
                // parent process
                if (current_user->cmds[i].pipe_type == NO_PIPE || current_user->cmds[i].pipe_type == F_RED_PIPE)
                    last_pid = pid;
                if (current_user->cmds[i].pipe_type == NUM_PIPE || current_user->cmds[i].pipe_type == ERR_NUM_PIPE)
                {
                    stop_pipe = true;
                }
                else
                {
                    stop_pipe = false;
                    current_user->cmds[i].is_piped = true;
                }
                current_user->cmds[i].is_exe = true;
                if (temp_id != 0)
                {
                    close(temp_fd_arr[temp_id][0]);
                    close(temp_fd_arr[temp_id][1]);
                }
                if (current_user->cmds[i].read_from != -1)
                    close_rd_user_pipe(user_info_arr, id, i);
                if (current_user->cmds[i].write_to != -1)
                    close_wr_user_pipe(user_info_arr, id, i);
                pre_pid = pid;
            }
        }
    }
    close_pipe(current_user->cmds);
    close_temp_pipe(temp_fd_arr);
    if (last_pid != -1)
        waitpid(last_pid, &status, 0);

    reduce_num_by_nl(current_user->cmds);
    current_user->cmds.erase(
        remove_if(
            current_user->cmds.begin(),
            current_user->cmds.end(),
            [](command const &p)
            { return p.is_piped; }),
        current_user->cmds.end());
    return;
}

/*
 * numbered pipe version
 */
void exe_bin(vector<command> &cmds)
{
    int status;
    // int stdin_copy = dup(STDIN_FILENO);
    int stdout_copy = dup(STDOUT_FILENO);
    vector<int *> temp_fd_arr;
    init_temp_fd(temp_fd_arr, 5); /*only support 5 number pipe in a line*/
    bool stop_pipe = true;
    pid_t last_pid = -1;
    size_t temp_id = 0;
    for (size_t i = 0; i < cmds.size(); i++)
    {
        if (!cmds[i].is_exe)
        {
            init_pipe(cmds[i].fd);
        }
    }
    for (size_t i = 0; i < cmds.size(); i++)
    {
        if (!cmds[i].is_exe)
        {
            collect_num_pipe_output(cmds, temp_fd_arr, temp_id, i);
            if ((cmds[i].pipe_type == NUM_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE))
                reduce_num_pipes(cmds, i);
            pid_t pid;
            pid = fork();
            if (pid == -1)
            {
                cerr << "fork error!\n";
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                //  child process
                close_unused_pipe_in_child(cmds, i);
                switch (cmds[i].pipe_type)
                {
                case NO_PIPE:
                    exe_command(stdout_copy, cmds, i, stop_pipe, temp_fd_arr[temp_id]);
                    break;
                case PIPE:
                    exe_pipe(stdout_copy, cmds, i, stop_pipe, temp_fd_arr[temp_id]);
                    break;
                case ERR_PIPE:
                    exe_err_pipe(stdout_copy, cmds, i, stop_pipe, temp_fd_arr[temp_id]);
                    break;
                case F_RED_PIPE:
                    exe_f_red(stdout_copy, cmds, i, temp_fd_arr[temp_id]);
                    break;
                case NUM_PIPE:
                    exe_num_pipe(stdout_copy, cmds, i, stop_pipe, temp_fd_arr[temp_id]);
                    break;
                case ERR_NUM_PIPE:
                    exe_err_num_pipe(stdout_copy, cmds, i, stop_pipe, temp_fd_arr[temp_id]);
                    break;
                default:
                    exit(EXIT_FAILURE);
                    break;
                }
            }
            else
            {
                // parent process
                if (cmds[i].pipe_type == NO_PIPE || cmds[i].pipe_type == F_RED_PIPE)
                    last_pid = pid;
                if (cmds[i].pipe_type == NUM_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE)
                {
                    stop_pipe = true;
                }
                else
                {
                    stop_pipe = false;
                    cmds[i].is_piped = true;
                }
                cmds[i].is_exe = true;
                if (temp_id != 0)
                {
                    close(temp_fd_arr[temp_id][0]);
                    close(temp_fd_arr[temp_id][1]);
                }
            }
        }
    }
    close_pipe(cmds);
    close_temp_pipe(temp_fd_arr);
    if (last_pid != -1)
        waitpid(last_pid, &status, 0);

    reduce_num_by_nl(cmds);
    cmds.erase(
        remove_if(
            cmds.begin(),
            cmds.end(),
            [](command const &p)
            { return p.is_piped; }),
        cmds.end());
    return;
}

// initialization
void init_env()
{
    setenv("PATH", "bin:.", true);
}

// build-in command
void print_env(const char *const para)
{
    char *cur_env;
    cur_env = getenv(para);
    if (cur_env != NULL)
        cout << cur_env << endl;
}

void print_users(vector<user_info> user_info_arr, const size_t id)
{
    size_t my_id = user_info_arr[id].id_num;
    sort(user_info_arr.begin(), user_info_arr.end(),
         [](user_info const &a, user_info const &b) -> bool
         { return a.id_num < b.id_num; });
    cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>" << endl;
    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        cout << user_info_arr[i].id_num << "\t"
             << user_info_arr[i].name << "\t"
             << inet_ntoa(user_info_arr[i].sock_addr_info.sin_addr)
             << ":" << ntohs(user_info_arr[i].sock_addr_info.sin_port);
        if (user_info_arr[i].id_num == my_id)
            cout << "\t<-me";
        cout << endl;
    }
}

void print_users(const user_info_shm_ver *user_info_arr, size_t id)
{
    cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>" << endl;
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num != 0)
        {
            cout << user_info_arr[i].id_num << "\t"
                 << user_info_arr[i].name << "\t"
                 << inet_ntoa(user_info_arr[i].sock_addr_info.sin_addr)
                 << ":" << ntohs(user_info_arr[i].sock_addr_info.sin_port);
            if (user_info_arr[i].id_num == id + 1)
                cout << "\t<-me";
            cout << endl;
        }
    }
}

void tell_to_other(const vector<user_info> &user_info_arr, const size_t sender_id, const size_t recv_id, string msg)
{
    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        // recv_id => id_num
        if (user_info_arr[i].id_num == recv_id)
        {
            int stdout_copy = dup(STDOUT_FILENO);
            dup2(user_info_arr[i].fd, STDOUT_FILENO);
            cout << "*** " << user_info_arr[sender_id].name << " told you ***:" << msg << endl;
            dup2(stdout_copy, STDOUT_FILENO);
            close(stdout_copy);
            return;
        }
    }
    cout << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
}

void tell_to_other(user_info_shm_ver *user_info_arr, const size_t sender_id, const size_t recv_id, string msg)
{
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num != 0)
        {
            if (user_info_arr[i].id_num == recv_id)
            {
                stringstream ss;
                string temp;
                ss << "*** " << user_info_arr[sender_id].name << " told you ***:" << msg << endl;
                getline(ss, temp);
                temp += "\n";
                strcpy(user_info_arr[i].broadcast_msg, temp.c_str());
                return;
            }
        }
    }
    cout << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
}

void change_name(vector<user_info> &user_info_arr, const size_t id, string input_name)
{
    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        if (i != id)
        {
            if (user_info_arr[i].name == input_name)
            {
                cout << "*** User \'" << input_name << "\' already exists. ***" << endl;
                return;
            }
        }
    }
    user_info_arr[id].name = input_name;
    broadcast(user_info_arr, CHANGE_NAME, id, "");
}

void change_name(user_info_shm_ver *user_info_arr, const size_t id, std::string input_name)
{
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num != 0)
        {
            if (i != id)
            {
                if (user_info_arr[i].name == input_name)
                {
                    cout << "*** User \'" << input_name << "\' already exists. ***" << endl;
                    return;
                }
            }
        }
    }
    strcpy(user_info_arr[id].name, input_name.c_str());
    broadcast(user_info_arr, CHANGE_NAME, id, "");
}

void broadcast(const vector<user_info> &user_info_arr, BROADCAST_TYPE_E br_type, size_t self_id, string msg)
{
    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        if (user_info_arr[i].is_closed)
            continue;
        int stdout_copy = dup(STDOUT_FILENO);
        dup2(user_info_arr[i].fd, STDOUT_FILENO);
        switch (br_type)
        {
        case LOG_IN:
            if (i == self_id)
            {
                cout << "****************************************\n"
                     << "** Welcome to the information server. **\n"
                     << "****************************************" << endl;
            }
            cout << "*** User \'"
                 << user_info_arr[self_id].name << "\' entered from "
                 << inet_ntoa(user_info_arr[self_id].sock_addr_info.sin_addr) << ":"
                 << ntohs(user_info_arr[self_id].sock_addr_info.sin_port) << ". ***" << endl;
            if (i == self_id)
                cout << "% " << flush;
            break;
        case LOG_OUT:
            cout << "*** User \'" << user_info_arr[self_id].name << "\' left. ***" << endl;
            break;
        case WR_USER_PIPE_BR:
            cout << "*** "
                 << user_info_arr[self_id].name << " (#"
                 << user_info_arr[self_id].id_num << ") just piped \'"
                 << user_info_arr[self_id].recv_input << "\' to "
                 << msg
                 << endl;
            break;
        case RD_USER_PIPE_BR:
            cout << "*** "
                 << user_info_arr[self_id].name
                 << " (#"
                 << user_info_arr[self_id].id_num << ") just received from "
                 << msg << " by \'"
                 << user_info_arr[self_id].recv_input << "\' ***" << endl;
            break;
        case YELL_BR:
            cout << "*** " << user_info_arr[self_id].name << " yelled ***:" << msg << endl;
            break;
        case CHANGE_NAME:
            cout << "*** User from "
                 << inet_ntoa(user_info_arr[self_id].sock_addr_info.sin_addr)
                 << ":" << ntohs(user_info_arr[self_id].sock_addr_info.sin_port)
                 << " is named \'"
                 << user_info_arr[self_id].name
                 << "\'. ***" << endl;
            break;
        default:
            break;
        }
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
    }
}

void broadcast(user_info_shm_ver *user_info_arr, BROADCAST_TYPE_E br_type, size_t self_id, std::string msg)
{
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num == 0)
            continue;
        stringstream ss;
        string temp;
        switch (br_type)
        {
        case LOG_IN:
            if (i != self_id)
            {
                ss << "*** User \'"
                   << user_info_arr[self_id].name << "\' entered from "
                   << inet_ntoa(user_info_arr[self_id].sock_addr_info.sin_addr) << ":"
                   << ntohs(user_info_arr[self_id].sock_addr_info.sin_port) << ". ***";
            }
            break;
        case LOG_OUT:
            if (i != self_id)
            {
                ss << "*** User \'" << user_info_arr[self_id].name << "\' left. ***" << endl;
            }
            break;
        case WR_USER_PIPE_BR:
            if (i != self_id)
            {
                ss << "*** "
                   << user_info_arr[self_id].name << " (#"
                   << user_info_arr[self_id].id_num << ") just piped \'"
                   << user_info_arr[self_id].recv_input << "\' to "
                   << msg;
            }
            break;
        case RD_USER_PIPE_BR:
            if (i != self_id)
            {
                ss << "*** "
                   << user_info_arr[self_id].name
                   << " (#"
                   << user_info_arr[self_id].id_num << ") just received from "
                   << msg << " by \'"
                   << user_info_arr[self_id].recv_input << "\' ***";
            }
            break;
        case YELL_BR:
            ss << "*** " << user_info_arr[self_id].name << " yelled ***:" << msg;
            break;
        case CHANGE_NAME:
            ss << "*** User from "
               << inet_ntoa(user_info_arr[self_id].sock_addr_info.sin_addr)
               << ":" << ntohs(user_info_arr[self_id].sock_addr_info.sin_port)
               << " is named \'"
               << user_info_arr[self_id].name
               << "\'. ***";
            break;
        default:
            break;
        }
        getline(ss, temp);
        if (!temp.empty())
        {
            temp += "\n";
            strcat(user_info_arr[i].broadcast_msg, temp.c_str());
        }
    }
}

void reset_logout_user(user_info_shm_ver *user_info_arr, size_t id)
{
    user_info_arr[id].id_num = 0;
    strcpy(user_info_arr[id].name, "(no name)");
    strcpy(user_info_arr[id].recv_input, "");
    strcpy(user_info_arr[id].broadcast_msg, "");
}

void clean_user_pipe(std::vector<user_info> &user_info_arr, size_t log_out_id)
{
    size_t out_id_num = user_info_arr[log_out_id].id_num;
    for (size_t i = 0; i < user_info_arr.size(); i++)
    {
        if (user_info_arr[i].user_pipe.find(out_id_num) != user_info_arr[i].user_pipe.end())
        {
            user_info_arr[i].user_pipe.erase(out_id_num);
        }
    }
}

void clean_user_pipe(user_info_shm_ver *user_info_arr, size_t log_out_id)
{
    for (size_t i = 0; i < 30; i++)
    {
        user_info_arr[log_out_id].user_pipe[i].send_to_id = 0;
    }
    size_t out_id_num = user_info_arr[log_out_id].id_num;
    for (size_t i = 0; i < 30; i++)
    {
        if (user_info_arr[i].id_num != 0)
        {
            for (size_t j = 0; j < 30; j++)
            {
                if (user_info_arr[i].user_pipe[j].send_to_id == out_id_num)
                {
                    user_info_arr[i].user_pipe[j].send_to_id = 0;
                }
            }
        }
    }
}

// functions for execute command
inline void init_pipe(int *fd)
{
    if (pipe(fd) == -1)
    {
        cerr << "pipe error\n";
        exit(EXIT_FAILURE);
    }
}

inline void reduce_num_pipes(vector<command> &number_pipes, int last)
{
    for (int i = 0; i <= last; i++)
    {
        if (number_pipes[i].pipe_num > 0)
            number_pipes[i].pipe_num--;
    }
}

inline void init_temp_fd(vector<int *> &temp_fd_arr, size_t s)
{
    for (size_t i = 0; i < s; i++)
    {
        int *temp_fd = new int[2];
        if (pipe(temp_fd) == -1)
        {
            cerr << "pipe error\n";
            exit(EXIT_FAILURE);
        }
        temp_fd_arr.push_back(temp_fd);
    }
}

void collect_num_pipe_output(vector<command> &cmds, vector<int *> &temp_fd_arr, size_t &temp_id, size_t i)
{
    bool is_new_temp = true;
    for (size_t j = 0; j < i; j++)
    {
        if (cmds[j].pipe_type == NUM_PIPE || cmds[j].pipe_type == ERR_NUM_PIPE)
        {
            if (!cmds[j].is_piped && cmds[j].pipe_num == 0)
            {
                if (is_new_temp)
                    temp_id++;
                is_new_temp = false;
                pid_t pid;
                pid = fork();
                if (pid == -1)
                {
                    cerr << "fork error!\n";
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)
                {
                    // cout << i << "th collecting from " << j << endl;
                    dup2(cmds[j].fd[0], STDIN_FILENO);
                    close(cmds[j].fd[0]);
                    close(cmds[j].fd[1]);
                    dup2(temp_fd_arr[temp_id][1], STDOUT_FILENO);
                    close(temp_fd_arr[temp_id][0]);
                    close(temp_fd_arr[temp_id][1]);
                    if (execlp("cat", "cat", NULL) == -1)
                    {
                        cerr << "cat error\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    cmds[j].is_piped = true;
                    close(cmds[j].fd[0]);
                    close(cmds[j].fd[1]);
                }
            }
        }
    }
}

inline void close_unused_pipe_in_child(vector<command> &cmds, size_t i)
{
    for (size_t j = 0; j < cmds.size(); j++)
    {
        if (i - 1 != j)
            close(cmds[j].fd[0]);
        if (i != j)
            close(cmds[j].fd[1]);
    }
}

inline void close_pipe(vector<command> &cmds)
{
    for (size_t i = 0; i < cmds.size(); i++)
    {
        switch (cmds[i].pipe_type)
        {
        case NO_PIPE:
        case PIPE:
        case ERR_PIPE:
        case F_RED_PIPE:
            close(cmds[i].fd[0]);
            close(cmds[i].fd[1]);
            break;
        case NUM_PIPE:
        case ERR_NUM_PIPE:
            if (cmds[i].is_piped)
            {
                close(cmds[i].fd[0]);
                close(cmds[i].fd[1]);
            }
            break;
        default:
            break;
        }
    }
}

inline void close_temp_pipe(vector<int *> &temp_fd_arr)
{
    for (size_t i = 0; i < temp_fd_arr.size(); i++)
    {
        close(temp_fd_arr[i][0]);
        close(temp_fd_arr[i][1]);
        free(temp_fd_arr[i]);
    }
}

inline void close_wr_user_pipe(vector<user_info> &user_info_arr, int id, int i)
{
    user_info *current_user = &(user_info_arr[id]);
    int recv_id = current_user->cmds[i].write_to;
    close(current_user->user_pipe[recv_id][1]);
}

inline void close_wr_user_pipe(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i)
{
    return;
}

inline void close_rd_user_pipe(vector<user_info> &user_info_arr, int id, int i)
{
    user_info *current_user = &(user_info_arr[id]);
    user_info *source_user = NULL;
    size_t current_id = current_user->id_num;
    size_t source_id = current_user->cmds[i].read_from;
    for (vector<user_info>::iterator it = user_info_arr.begin(); it != user_info_arr.end(); it++)
    {
        if (source_id == (*it).id_num)
        {
            source_user = &(*it);
            break;
        }
    }
    close(source_user->user_pipe[current_id][0]);
    source_user->user_pipe.erase(current_id);
}

inline void close_rd_user_pipe(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i)
{
    user_info_shm_ver *current_user = &(user_info_arr[id]);
    user_info_shm_ver *source_user;
    size_t current_id = current_user->id_num;
    size_t source_id = cmds[cmd_i].read_from;
    for (size_t i = 0; i < 30; i++)
    {
        if (source_id == user_info_arr[i].id_num)
        {
            source_user = &(user_info_arr[i]);
            break;
        }
    }
    for (size_t i = 0; i < 30; i++)
    {
        if (current_id == source_user->user_pipe[i].send_to_id)
        {
            source_user->user_pipe[i].send_to_id = 0;
            break;
        }
    }
}

inline void reduce_num_by_nl(vector<command> &cmds)
{
    switch (cmds.back().pipe_type)
    {
    case NO_PIPE:
    case PIPE:
    case ERR_PIPE:
    case F_RED_PIPE:
        for (size_t i = 0; i < cmds.size(); i++)
        {
            if (cmds[i].pipe_num > 0)
                cmds[i].pipe_num--;
        }
        break;
    case NUM_PIPE:
    case ERR_NUM_PIPE:
        break;
    default:
        break;
    }
}

inline int init_rd_user_pp(vector<user_info> &user_info_arr, int id, int cmd_i)
{
    user_info *current_user = &(user_info_arr[id]);
    user_info *source_user;
    size_t current_id = current_user->id_num;
    size_t source_id = current_user->cmds[cmd_i].read_from;
    bool is_user_existed = false;
    stringstream ss;
    string source_info;
    for (vector<user_info>::iterator it = user_info_arr.begin(); it != user_info_arr.end(); it++)
    {
        if (source_id == (*it).id_num)
        {
            ss << it->name << " (#" << it->id_num << ")";
            getline(ss, source_info);
            is_user_existed = true;
            source_user = &(*it);
            break;
        }
    }
    if (!is_user_existed)
    {
        cout << "*** Error: user #" << source_id << " does not exist yet. ***" << endl;
        return -1;
    }
    if (source_user->user_pipe.find(current_id) == source_user->user_pipe.end())
    {
        // not found
        cout << "*** Error: the pipe #" << source_id << "->#" << current_user->id_num << " does not exist yet. ***" << endl;
        return -1;
    }
    else
    {
        // found
        broadcast(user_info_arr, RD_USER_PIPE_BR, id, source_info);
        return 1;
    }
}

inline int init_wr_user_pp(vector<user_info> &user_info_arr, int id, int cmd_i)
{
    user_info *current_user = &(user_info_arr[id]);
    size_t recv_id = current_user->cmds[cmd_i].write_to;
    bool is_user_existed = false;
    stringstream ss;
    string recv_info;
    for (vector<user_info>::iterator it = user_info_arr.begin(); it != user_info_arr.end(); it++)
    {
        if (recv_id == (*it).id_num)
        {
            ss << it->name << " (#" << it->id_num << ") ***";
            getline(ss, recv_info);
            is_user_existed = true;
            break;
        }
    }
    if (!is_user_existed)
    {
        cout << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
        return -1;
    }
    if (current_user->user_pipe.find(recv_id) == current_user->user_pipe.end())
    {
        // not found
        int *temp_fd = new int[2];
        init_pipe(temp_fd);
        current_user->user_pipe.insert(pair<size_t, int *>(recv_id, temp_fd));
        broadcast(user_info_arr, WR_USER_PIPE_BR, id, recv_info);
        return 1;
    }
    else
    {
        // found
        cout << "*** Error: the pipe #" << current_user->id_num << "->#" << recv_id << " already exists. ***" << endl;
        return -1;
    }
}
inline int init_rd_user_pp(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i)
{
    user_info_shm_ver *current_user = &(user_info_arr[id]);
    user_info_shm_ver *source_user;
    size_t current_id = current_user->id_num;
    size_t source_id = cmds[cmd_i].read_from;
    bool is_user_existed = false;
    stringstream ss;
    string source_info;
    for (size_t i = 0; i < 30; i++)
    {
        if (source_id == user_info_arr[i].id_num)
        {
            ss << user_info_arr[i].name << " (#" << user_info_arr[i].id_num << ")";
            getline(ss, source_info);
            is_user_existed = true;
            source_user = &(user_info_arr[i]);
            break;
        }
    }
    if (!is_user_existed)
    {
        cout << "*** Error: user #" << source_id << " does not exist yet. ***" << endl;
        return -1;
    }
    bool is_user_pipe_existed = false;
    for (size_t i = 0; i < 30; i++)
    {
        if (current_id == source_user->user_pipe[i].send_to_id)
        {
            is_user_pipe_existed = true;
            break;
        }
    }
    if (is_user_pipe_existed)
    {
        // found
        cout << "*** "
             << user_info_arr[id].name
             << " (#"
             << user_info_arr[id].id_num << ") just received from "
             << source_info << " by \'"
             << user_info_arr[id].recv_input << "\' ***" << endl;
        broadcast(user_info_arr, RD_USER_PIPE_BR, id, source_info);
        return 1;
    }
    else
    {
        // not found
        cout << "*** Error: the pipe #" << source_id << "->#" << current_user->id_num << " does not exist yet. ***" << endl;
        return -1;
    }
}

inline int init_wr_user_pp(user_info_shm_ver *user_info_arr, int id, const vector<command> &cmds, int cmd_i)
{
    user_info_shm_ver *current_user = &(user_info_arr[id]);
    size_t recv_id = cmds[cmd_i].write_to;
    bool is_user_existed = false;
    stringstream ss;
    string recv_info;
    for (size_t i = 0; i < 30; i++)
    {
        if (recv_id == user_info_arr[i].id_num)
        {
            ss << user_info_arr[i].name << " (#" << user_info_arr[i].id_num << ") ***";
            getline(ss, recv_info);
            is_user_existed = true;
            break;
        }
    }
    if (!is_user_existed)
    {
        cout << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
        return -1;
    }
    bool is_user_pipe_existed = false;
    for (size_t i = 0; i < 30; i++)
    {
        if (recv_id == current_user->user_pipe[i].send_to_id)
        {
            is_user_pipe_existed = true;
            break;
        }
    }
    if (is_user_pipe_existed)
    {
        // found
        cout << "*** Error: the pipe #" << current_user->id_num << "->#" << recv_id << " already exists. ***" << endl;
        return -1;
    }
    else
    {
        // not found
        for (size_t i = 0; i < 30; i++)
        {
            if (current_user->user_pipe[i].send_to_id == 0)
            {
                current_user->user_pipe[i].send_to_id = recv_id;
                break;
            }
        }
        cout << "*** "
             << user_info_arr[id].name << " (#"
             << user_info_arr[id].id_num << ") just piped \'"
             << user_info_arr[id].recv_input << "\' to "
             << recv_info << endl;
        broadcast(user_info_arr, WR_USER_PIPE_BR, id, recv_info);
        return 1;
    }
}

inline bool is_rd_user_pp_exist(command cmd)
{
    return cmd.read_from != -1;
}

inline bool is_wr_user_pp_exist(command cmd)
{
    return cmd.write_to != -1;
}

char **vector_to_c_str_arr(vector<string> cmd)
{
    char **arr = (char **)malloc((cmd.size() + 1) * sizeof(char *));
    for (size_t i = 0; i < cmd.size(); i++)
    {
        // arr[i] = strdup((cmd[i]).c_str());
        size_t slen = strlen((cmd[i]).c_str());
        char *temp = (char *)malloc(slen + 1);
        if (temp == NULL)
        {
            return NULL;
        }
        memcpy(temp, (cmd[i]).c_str(), slen + 1);
        arr[i] = temp;
    }
    arr[cmd.size()] = NULL;
    return arr;
}

// execute command
// inline int get_read_user_pipe_id(const command &cmd)
// {
//     return cmd.write_to;
// }

// inline int get_write_user_pipe_id()
// {
// }

/*
 * multi proc version
 */
void exe_command(int stdout_copy, vector<command> &cmds, user_info_shm_ver *user_info_arr, int id, int i, bool stop_pipe, int temp_fd[])
{
    user_info_shm_ver *current_user = &(user_info_arr[id]);
    if (cmds[i].read_from != -1)
    {
        // open FIFO user pipe
        stringstream fifo_ss;
        string fifo_name;
        fifo_ss << "user_pipe/" << cmds[i].read_from << "to" << current_user->id_num;
        getline(fifo_ss, fifo_name);
        int fifo_usr_rd_fd = open(fifo_name.c_str(), O_RDONLY);
        if (fifo_usr_rd_fd == -1)
        {
            perror("open");
        }
        dup2(fifo_usr_rd_fd, STDIN_FILENO);
        close(fifo_usr_rd_fd);
        unlink(fifo_name.c_str());
    }
    else
    {
        if (stop_pipe)
        {
            if (dup2(temp_fd[0], STDIN_FILENO) == -1)
                perror("dup2");
        }
        else
        {
            dup2(cmds[i - 1].fd[0], STDIN_FILENO);
            close(cmds[i - 1].fd[0]);
        }
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    // close(current_user->cmds[i].fd[0]);
    if (cmds[i].write_to != -1)
    {
        // open FIFO pipe

        stringstream fifo_ss;
        string fifo_name;
        fifo_ss << "user_pipe/" << current_user->id_num << "to" << cmds[i].write_to;
        getline(fifo_ss, fifo_name);
        mkfifo(fifo_name.c_str(), 0777);
        int fifo_usr_rd_fd = open(fifo_name.c_str(), O_WRONLY);
        if (fifo_usr_rd_fd == -1)
        {
            perror("open");
        }
        dup2(fifo_usr_rd_fd, STDOUT_FILENO);
        close(fifo_usr_rd_fd);
        close(cmds[i].fd[1]);
    }
    else
    {
        if (cmds[i].pipe_type == NO_PIPE)
        {
            dup2(stdout_copy, STDOUT_FILENO);
        }
        else
        {
            if (dup2(cmds[i].fd[1], STDOUT_FILENO) == -1)
                perror("dup2");
            if (cmds[i].pipe_type == ERR_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE)
                dup2(cmds[i].fd[1], STDERR_FILENO);
            close(cmds[i].fd[1]);
        }
    }
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

/*
 * User pipe version
 */
void exe_command(int stdout_copy, vector<user_info> &user_info_arr, int id, int i, bool stop_pipe, int temp_fd[])
{
    // to do: dup2 error handle, if optimization
    user_info *current_user = &(user_info_arr[id]);
    size_t current_id = current_user->id_num;
    if (current_user->cmds[i].read_from != -1)
    {
        user_info *source_user = NULL;
        size_t source_id = current_user->cmds[i].read_from;
        for (vector<user_info>::iterator it = user_info_arr.begin(); it != user_info_arr.end(); it++)
        {
            if (source_id == (*it).id_num)
            {
                source_user = &(*it);
                break;
            }
        }
        dup2(source_user->user_pipe[current_id][0], STDIN_FILENO);
        close(source_user->user_pipe[current_id][0]);
    }
    else
    {
        if (stop_pipe)
        {
            if (dup2(temp_fd[0], STDIN_FILENO) == -1)
                perror("dup2");
        }
        else
        {
            dup2(current_user->cmds[i - 1].fd[0], STDIN_FILENO);
            close(current_user->cmds[i - 1].fd[0]);
        }
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    // close(current_user->cmds[i].fd[0]);
    if (current_user->cmds[i].write_to != -1)
    {
        int recv_id = current_user->cmds[i].write_to;
        dup2(current_user->user_pipe[recv_id][1], STDOUT_FILENO);
        close(current_user->user_pipe[recv_id][1]);
    }
    else
    {
        if (current_user->cmds[i].pipe_type == NO_PIPE)
        {
            dup2(stdout_copy, STDOUT_FILENO);
        }
        else
        {
            if (dup2(current_user->cmds[i].fd[1], STDOUT_FILENO) == -1)
                perror("dup2");
            if (current_user->cmds[i].pipe_type == ERR_PIPE || current_user->cmds[i].pipe_type == ERR_NUM_PIPE)
                dup2(current_user->cmds[i].fd[1], STDERR_FILENO);
            close(current_user->cmds[i].fd[1]);
        }
    }
    char **args = vector_to_c_str_arr(current_user->cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

// numbered pipe version => to do: merge exe_* function
void exe_command(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        if (dup2(temp_fd[0], STDIN_FILENO) == -1)
            perror("dup2");
    }
    else
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO);
        close(cmds[i - 1].fd[0]);
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    close(cmds[i].fd[0]);
    close(cmds[i].fd[1]);
    dup2(stdout_copy, STDOUT_FILENO);
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

void exe_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        dup2(temp_fd[0], STDIN_FILENO);
    }
    else
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO);
        close(cmds[i - 1].fd[0]);
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    if (i == 0)
    {
        close(cmds[0].fd[0]);
    }
    dup2(cmds[i].fd[1], STDOUT_FILENO); // stdout refer to p2[1]
    close(cmds[i].fd[1]);
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

void exe_err_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        dup2(temp_fd[0], STDIN_FILENO);
    }
    else
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO);
        close(cmds[i - 1].fd[0]);
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    if (i == 0)
    {
        close(cmds[0].fd[0]);
    }
    dup2(cmds[i].fd[1], STDERR_FILENO); // stderr refer to p2[1]
    dup2(cmds[i].fd[1], STDOUT_FILENO); // stdout refer to p2[1]
    close(cmds[i].fd[1]);
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

void exe_f_red(int stdout_copy, vector<command> &cmds, int i, int temp_fd[])
{
    close(temp_fd[0]);
    close(temp_fd[1]);
    close(cmds[i].fd[0]);
    close(cmds[i].fd[1]);
    if (i > 0)
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO); // stdin refer to p1[0]
        close(cmds[i - 1].fd[0]);
    }
    else if (i == 0)
        cerr << "file redirection error!\n";
    int fd = open(cmds[i].cmd[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0775);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    if (execlp("cat", "cat", NULL) == -1)
    {
        cerr << "file redirection failed!\n";
        exit(EXIT_FAILURE);
    }
}

void exe_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        dup2(temp_fd[0], STDIN_FILENO);
    }
    else
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO);
        close(cmds[i - 1].fd[0]);
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    dup2(cmds[i].fd[1], STDOUT_FILENO); // stdout refer to p2[1]
    close(cmds[i].fd[1]);
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

void exe_err_num_pipe(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        dup2(temp_fd[0], STDIN_FILENO);
    }
    else
    {
        dup2(cmds[i - 1].fd[0], STDIN_FILENO);
        close(cmds[i - 1].fd[0]);
    }
    close(temp_fd[0]);
    close(temp_fd[1]);
    if (i == 0)
    {
        close(cmds[0].fd[0]);
    }
    dup2(cmds[i].fd[1], STDERR_FILENO); // stdout refer to p2[1]
    dup2(cmds[i].fd[1], STDOUT_FILENO); // stdout refer to p2[1]
    close(cmds[i].fd[1]);
    char **args = vector_to_c_str_arr(cmds[i].cmd);
    if (execvp(args[0], args) == -1)
    {
        // perror("Error: ");
        cerr << "Unknown command: [" << args[0] << "].\n";
        exit(EXIT_FAILURE);
    }
}

// debug
void print_cmds(vector<command> cmds)
{
    for (size_t i = 0; i < cmds.size(); i++)
    {
        for (size_t j = 0; j < cmds[i].cmd.size(); j++)
        {
            cout << "{" << cmds[i].cmd[j] << "} ";
        }
        cout << cmds[i].which_type();
        switch (cmds[i].pipe_type)
        {
        case NUM_PIPE:
        case ERR_NUM_PIPE:
            cout << " " << cmds[i].pipe_num;
            break;
        default:
            break;
        }
        if (cmds[i].read_from != -1)
            cout << " read: " << cmds[i].read_from;
        if (cmds[i].write_to != -1)
            cout << " write: " << cmds[i].write_to;
        if (cmds[i].is_exe)
            cout << " exed";
        if (cmds[i].is_piped)
            cout << " piped";
        cout << endl;
    }
}

// command class member function
string command::which_type()
{
    switch (pipe_type)
    {
    case NO_PIPE:
        return "NO_PIPE";
    case PIPE:
        return "PIPE";
    case ERR_PIPE:
        return "ERR_PIP";
    case F_RED_PIPE:
        return "F_RED_PIPE";
    case NUM_PIPE:
        return "NUM_PIPE";
    case ERR_NUM_PIPE:
        return "ERR_NUM_PIPE";
    default:
        return "error";
    }
}