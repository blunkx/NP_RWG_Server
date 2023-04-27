#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include <fstream>
#include <regex>

#include <sys/stat.h>
#include <unistd.h>

#include "command.h"

typedef enum BUILT_IN_COM_T
{
    NOT_BUILT_IN,
    SETENV,
    PRINTENV,
    EXIT,
    WHO,
    TELL,
    YELL,
    NAME,
} BUILT_IN_COM_E;

// for single user
void parser(std::string &input, std::vector<command> &number_pipes);

// for select
void parser(std::string &input, std::vector<user_info> &user_info_arr, size_t id);

// for multi proc
void parser(std::string &input, std::vector<command> &cmds, user_info_shm_ver *user_info_arr, size_t id);

#endif
