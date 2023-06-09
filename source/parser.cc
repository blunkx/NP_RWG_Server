#include "parser.h"

using namespace std;

BUILT_IN_COM_E is_built_in_command(const vector<string> tokens);
void parse_user_pipe(command &cmd);
void split_by_pipe(vector<string> &tokens, vector<command> &cmds);
void print_str_ascii(const std::string &input);
string handle_tell_msg(string input);
string handle_yell_msg(string input);

void parser(string &input, vector<command> &cmds)
{
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
    if (input.empty())
        return;
    vector<string> tokens;
    istringstream iss(input);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));
    switch (is_built_in_command(tokens))
    {
    case NOT_BUILT_IN:
        split_by_pipe(tokens, cmds);
        exe_bin(cmds);
        break;
    case SETENV:
        setenv(tokens[1].c_str(), tokens[2].c_str(), true);
        break;
    case PRINTENV:
        print_env(tokens[1].c_str());
        break;
    case EXIT:
        exit(EXIT_SUCCESS);
        break;
    default:
        exit(EXIT_FAILURE);
    }
}

void parser(string &input, vector<user_info> &user_info_arr, size_t id)
{
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
    if (input.empty())
        return;
    vector<string> tokens;
    istringstream iss(input);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));
    user_info_arr[id].recv_input = input;
    switch (is_built_in_command(tokens))
    {
    case NOT_BUILT_IN:
        split_by_pipe(tokens, user_info_arr[id].cmds);
        exe_bin(user_info_arr, id);
        break;
    case SETENV:
        user_info_arr[id].env_var[tokens[1]] = tokens[2];
        break;
    case PRINTENV:
        print_env(tokens[1].c_str());
        break;
    case EXIT:
        user_info_arr[id].is_closed = true;
        clean_user_pipe(user_info_arr, id); // remove all user pipe for log out user
        broadcast(user_info_arr, LOG_OUT, id, "");
        break;
    case WHO:
        print_users(user_info_arr, id);
        break;
    case TELL:
        tell_to_other(user_info_arr, id, stoi(tokens[1]), handle_tell_msg(input));
        break;
    case YELL:
        broadcast(user_info_arr, YELL_BR, id, handle_yell_msg(input));
        break;
    case NAME:
        change_name(user_info_arr, id, tokens[1]);
        break;
    default:
        exit(EXIT_FAILURE);
    }
}

void parser(string &input, vector<command> &cmds, user_info_shm_ver *user_info_arr, size_t id)
{
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
    if (input.empty())
        return;
    vector<string> tokens;
    istringstream iss(input);
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));
    strcpy(user_info_arr[id].recv_input, input.c_str());
    switch (is_built_in_command(tokens))
    {
    case NOT_BUILT_IN:
        split_by_pipe(tokens, cmds);
        exe_bin(cmds, user_info_arr, id);
        break;
    case SETENV:
        setenv(tokens[1].c_str(), tokens[2].c_str(), true);
        break;
    case PRINTENV:
        print_env(tokens[1].c_str());
        break;
    case EXIT:
        clean_user_pipe(user_info_arr, id);
        user_info_arr[id].id_num = 0;
        broadcast(user_info_arr, LOG_OUT, id, "");
        reset_logout_user(user_info_arr, id);
        close(user_info_arr[id].fd);
        exit(EXIT_SUCCESS);
        break;
    case WHO:
        print_users(user_info_arr, id);
        break;
    case TELL:
        tell_to_other(user_info_arr, id, stoi(tokens[1]), handle_tell_msg(input));
        break;
    case YELL:
        broadcast(user_info_arr, YELL_BR, id, handle_yell_msg(input));
        break;
    case NAME:
        change_name(user_info_arr, id, tokens[1]);
        break;
    default:
        exit(EXIT_FAILURE);
    }
}

BUILT_IN_COM_E is_built_in_command(const vector<string> tokens)
{
    /*
    setenv [var] [value]
    printenv [var]
    exit
    who
    tell [user id] [message]
    yell [message]
    name [new name]
    */
    if (tokens[0] == "setenv" && tokens.size() == 3)
        return SETENV;
    else if (tokens[0] == "printenv" && tokens.size() == 2)
        return PRINTENV;
    else if (tokens[0] == "exit" && tokens.size() == 1)
        return EXIT;
    else if (tokens[0] == "who" && tokens.size() == 1)
        return WHO;
    else if (tokens[0] == "tell" && tokens.size() > 2)
        return TELL;
    else if (tokens[0] == "yell" && tokens.size() > 1)
        return YELL;
    else if (tokens[0] == "name" && tokens.size() == 2)
        return NAME;
    else
        return NOT_BUILT_IN;
}

void parse_user_pipe(command &cmd)
{
    vector<size_t> removed_id;
    for (size_t i = 0; i < cmd.cmd.size(); i++)
    {
        if (std::regex_match((cmd.cmd[i]), std::regex("\\>\\d+")))
        {
            removed_id.push_back(i);
            cmd.write_to = stoi((cmd.cmd[i]).erase(0, 1));
        }
        else if (std::regex_match((cmd.cmd[i]), std::regex("\\<\\d+")))
        {
            removed_id.push_back(i);
            cmd.read_from = stoi((cmd.cmd[i]).erase(0, 1));
        }
    }
    for (size_t i = 0; i < removed_id.size(); i++)
    {
        cmd.cmd.erase(next(cmd.cmd.begin(), removed_id[i]));
    }
}

void split_by_pipe(vector<string> &tokens, vector<command> &cmds)
{
    vector<string>::iterator pre_end = tokens.begin();
    for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); it++)
    {
        command temp;
        if (*it == "|")
        {
            temp.pipe_type = PIPE;
            vector<string> temp_cmd(pre_end, it);
            temp.cmd = temp_cmd;
            parse_user_pipe(temp);
            cmds.push_back(temp);
            pre_end = it + 1;
        }
        else if (*it == "!")
        {
            temp.pipe_type = ERR_PIPE;
            vector<string> temp_cmd(pre_end, it);
            temp.cmd = temp_cmd;
            parse_user_pipe(temp);
            cmds.push_back(temp);
            pre_end = it + 1;
        }
        else if (*it == ">")
        {
            temp.pipe_type = PIPE;
            vector<string> temp_cmd(pre_end, it);
            temp.cmd = temp_cmd;
            cmds.push_back(temp);

            command temp_frd;
            temp_frd.pipe_type = F_RED_PIPE;
            vector<string> fname(it + 1, it + 2);
            temp_frd.cmd = fname;
            cmds.push_back(temp_frd);
            it++;
            pre_end = it + 1;
        }
        else if (std::regex_match((*it), std::regex("\\|\\d+")))
        {
            temp.pipe_type = NUM_PIPE;
            vector<string> temp_cmd(pre_end, it);
            temp.cmd = temp_cmd;
            temp.pipe_num = stoi((*it).erase(0, 1));
            parse_user_pipe(temp);
            cmds.push_back(temp);
            pre_end = it + 1;
        }
        else if (std::regex_match((*it), std::regex("\\!\\d+")))
        {
            temp.pipe_type = ERR_NUM_PIPE;
            vector<string> temp_cmd(pre_end, it);
            temp.cmd = temp_cmd;
            temp.pipe_num = stoi((*it).erase(0, 1));
            parse_user_pipe(temp);
            cmds.push_back(temp);
            pre_end = it + 1;
        }
        else if (it == tokens.end() - 1)
        {
            temp.pipe_type = NO_PIPE;
            vector<string> temp_cmd(pre_end, tokens.end());
            temp.cmd = temp_cmd;
            parse_user_pipe(temp);
            cmds.push_back(temp);
        }
    }
}

void print_str_ascii(const std::string &input)
{
    std::cout << "str length:" << input.size() << std::endl;
    for (size_t i = 0; i < input.size(); i++)
    {
        std::cout << i << " " << int(input[i]) << std::endl;
    }
}

string handle_tell_msg(string input)
{
    stringstream ss(input);
    string useless;
    ss >> useless;
    ss >> useless;
    string msg;
    getline(ss, msg);
    return msg;
}

string handle_yell_msg(string input)
{
    stringstream ss(input);
    string useless;
    ss >> useless;
    string msg;
    getline(ss, msg);
    return msg;
}