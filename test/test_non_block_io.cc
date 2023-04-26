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

int main(int argc, char *const argv[])
{
    char buf[20];
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    while (true)
    {
        sleep(1);
        int numRead = read(0, buf, 4);
        if (numRead > 0)
        {
            printf("You said: %s", buf);
        }
        else
            std::cout << "N\n"
                      << std::flush;
    }
    return 0;
}
