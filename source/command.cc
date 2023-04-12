#include "command.h"

using namespace std;

void exe_bin(vector<command> &cmds)
{
    bool debug_output = false;
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
    if (debug_output)
        cout << "==========================" << endl;
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
void exe_command(int stdout_copy, vector<command> &cmds, int i, bool stop_pipe, int temp_fd[])
{
    if (stop_pipe)
    {
        // cout << i << "th catch form np stdin" << endl;
        // cout << temp_fd << endl;
        // dup2(temp_fd[0], STDIN_FILENO);
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
        if (cmds[i].pipe_type == NUM_PIPE || cmds[i].pipe_type == ERR_NUM_PIPE)
            cout << " " << cmds[i].pipe_num;
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