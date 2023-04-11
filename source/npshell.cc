#include "npshell.h"

void exe_shell()
{
    init_env();
    string input;
    vector<command> cmds;
    while (true)
    {
        cout << "% ";
        if (!getline(cin, input).eof())
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
