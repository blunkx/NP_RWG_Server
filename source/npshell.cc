#include "npshell.h"

void exe_shell()
{
    init_env();
    std::string input;
    std::vector<command> cmds;
    while (true)
    {
        std::cout << "% ";
        if (!std::getline(std::cin, input).eof())
        {
            parser(input, cmds);
        }
        else
        {
            parser(input, cmds);
            break;
        }
    }
}
