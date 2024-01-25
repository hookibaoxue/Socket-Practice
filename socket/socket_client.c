#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // 创建客户端套接字
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket create failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");   // 目标服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    
    // 连接服务端
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    printf("connect success.................");
    char strs[100];
    int write_ret;
    while (1) {
        printf("input your message:\n");
        fgets(strs, sizeof(strs), stdin);
        write_ret = write(client_fd, strs, sizeof(strs));
        if (write_ret < 0) {
            perror("write failed");
            break;
        }

        // 添加个主动退出的指令
        if (strncmp(strs, "quit", 4) == 0) {
            break;
        }
    }

    close(client_fd);
    return 0;
}