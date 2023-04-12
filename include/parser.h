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
} BUILT_IN_COM_E;

void parser(std::string &input, std::vector<command> &number_pipes);

#endif
