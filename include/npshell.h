#ifndef _NPSHELL_H_
#define _NPSHELL_H_
#include <iostream>
#include <string>

#include "command.h"
#include "parser.h"

// for single user
void exe_shell();

// for multi proc
void exe_shell(user_info_shm_ver *user_info_arr, size_t id);

#endif
