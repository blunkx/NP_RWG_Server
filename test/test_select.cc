#include <iostream>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

#define SERV_IP "127.1.2.3"
#define SERV_PORT 8888
#define MAX_CONN 1024

int main()
{
    sockaddr_in servaddr, clitaddr;
    sockaddr_in clit_info[MAX_CONN]; // 存放成功連接的客戶端地址信息
    int client[1024];                // 存放成功連接的文件描述符
    char buf[1024];                  // 讀寫緩衝區
    int lfd;                         // 用於監聽
    int connfd;                      // 連接描述符
    int readyfd;                     // 保存select返回值
    int maxfd = 0;                   // 保存最大文件描述符
    int maxi = 0;                    // maxi反映了client中最後一個成功連接的文件描述符的索引
    socklen_t addr_len = sizeof(clitaddr);
    ;

    fd_set allset; // 存放所有可以被監控的文件描述符
    fd_set rset;

    FD_ZERO(&allset);
    FD_ZERO(&rset);

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cout << "creat socket fault : " << strerror(errno) << endl;
        return 0;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERV_IP);

    if (bind(lfd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        cout << "bind fault : " << strerror(errno) << endl;
        return 0;
    }

    if (listen(lfd, 128) == -1)
    {
        cout << "listen fault : " << strerror(errno) << endl;
        return 0;
    }

    maxfd = lfd; // 此時只用監控lfd，因此lfd就是最大文件描述符

    // 初始化client數組
    for (int i = 0; i < MAX_CONN; i++)
        client[i] = -1;

    FD_SET(lfd, &allset);

    cout << "Init Success ! " << endl;
    cout << "host ip : " << inet_ntoa(servaddr.sin_addr) << "  port : " << ntohs(servaddr.sin_port) << endl;

    cout << "Waiting for connections ... " << endl;

    while (1)
    {
        rset = allset;                                        // rset作爲select參數時，表示需要監控的所有文件描述符集合，select返回時，rset中存放的是成功監控的文件描述符。因此在select前後rset是可能改變的，所以在調用select前將rset置爲所有需要被監控的文件描述符的集合，也就是allset
        readyfd = select(maxfd + 1, &rset, NULL, NULL, NULL); // 服務端只考慮讀的情況
        // 執行到這裏，說明select返回，返回值保存在readyfd中，表示有多少個文件描述符被監控成功
        if (readyfd == -1)
        {
            cout << "select fault : " << strerror(errno) << endl;
            return 0;
        }

        if (FD_ISSET(lfd, &rset)) // 監聽描述符監控成功，說明有連接請求
        {
            int i = 0;
            connfd = accept(lfd, (sockaddr *)&clitaddr, &addr_len); // 處理新連接，此時accept直接可以返回而不用一直阻塞
            if (connfd == -1)
            {
                cout << "accept fault : " << strerror(errno) << endl;
                continue;
            }
            cout << inet_ntoa(clitaddr.sin_addr) << ":" << ntohs(clitaddr.sin_port) << " connected ...  " << endl;
            // 成功連接後，就將connfd加入監控描述符表中
            FD_SET(connfd, &allset);

            for (; i < MAX_CONN; i++)
            {
                if (client[i] == -1)
                {
                    client[i] = connfd;
                    clit_info[i] = clitaddr;
                    break;
                }
            }

            if (connfd > maxfd)
                maxfd = connfd; // 更新最大文件描述符
            if (i > maxi)
                maxi = i;

            readyfd--;
            if (readyfd == 0)
                continue; // 如果只有lfd被監控成功，那麼就重新select
        }
        // 處理lfd之外監控成功的文件描述符，進行輪詢
        for (int i = 0; i <= maxi; i++)
        {
            if (client[i] == -1)
                continue;                   // 等於-1說明這個描述符已經無效
            if (FD_ISSET(client[i], &rset)) // 在client數組中尋找是否有被監控成功的文件描述符
            {
                // 此時說明client[i]對於的文件描述符監控成功，有消息發來，直接讀取即可
                int readcount = read(client[i], buf, sizeof(buf));
                if (readcount == 0) // 對方客戶端關閉
                {
                    close(client[i]);           // 關閉描述符
                    FD_CLR(client[i], &allset); // 將該描述符從描述符集合中去除
                    client[i] = -1;             // 相應位置置爲-1，表示失效

                    cout << inet_ntoa(clit_info[i].sin_addr) << ":" << ntohs(clit_info[i].sin_port) << " exit ... " << endl;
                }
                else if (readcount == -1)
                {
                    cout << "read fault : " << strerror(errno) << endl;
                    continue;
                }
                else
                {
                    cout << "(From " << inet_ntoa(clit_info[i].sin_addr) << ":" << ntohs(clit_info[i].sin_port) << ")";
                    for (int j = 0; j < readcount; j++)
                        cout << buf[j];
                    cout << endl;
                    for (int j = 0; j < readcount; j++)
                        buf[j] = toupper(buf[j]);
                    write(client[i], buf, readcount);
                }
                readyfd--;
                if (readyfd == 0)
                    break;
            }
        }
    }
    close(lfd);
    return 0;
}