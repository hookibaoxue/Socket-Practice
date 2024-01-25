#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;// 服务端监听套接字，客户端套接字
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    //1.创建套接字
    /*
        #include <sys/types.h>
        #include <sys/socket.h>

        int socket(int domain, int type, int protocol);
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);// 创建TCP套接字
    if (server_fd < 0) {
        perror("socket create failed");
        exit(EXIT_FAILURE);
    }

    /*
        #include <string.h>

        void *memset(void *s, int c, size_t n);
    */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //如果需要与非本地地址客户端交互，需要用外部ip地址，不能用回环地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);

    //2.绑定套接字
    int bind_ret = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_ret < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //3.监听套接字
    int listen_ret = listen(server_fd, 8);//最大连接数为8
    if (listen_ret < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    //4.接收客户端连接
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    //5.读取或者发送消息
    char get_strs[100];
    size_t str_len = sizeof(get_str);
    while (1) {
        memset(get_strs, 0, str_len);
        ssize_t bytes_received = recv(client_fd, get_strs, str_len, 0);
        if (bytes_received == 0) {
            printf("client close socket");
            break;
        } else {
            perror("recv failed");
            break;
        }

        printf("server get message from client: %s\n", get_strs);
        const char* response = "server response message";
        send(client_fd, response, strlen(response), 0);
    }

    //6.关闭套接字
    close(server_fd);
    close(client_fd);
    return 0;
}