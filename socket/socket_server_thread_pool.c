#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8888
#define DEFAULT_COUNT 4

typedef struct {
    int client_socket;
} Task;

/*
  条件变量是利用线程间共享的全局变量进行同步的一种机制，主要包括两个动作：一个线程等待"条件变量的条件成立"而挂起；
  另一个线程使"条件成立"（给出条件成立信号）。为了防止竞争，条件变量的使用总是和一个互斥锁结合在一起。  
*/

typedef struct {
    Task *tasks;      // 线程
    int capacity;     // 线程池容量
    int front;       // 指向线程池第一个线程
    int rear;        // 指向最后一个线程的后一位
    int size;        // 已使用的线程数量

    pthread_mutex_t lock;       // 互斥锁
    pthread_cond_t not_empty;   // 条件变量 线程池非空 
    pthread_cond_t not_full;    // 条件变量 线程池未满
} ThreadPool;

void init_thread_pool(ThreadPool *pool, int capacity) {
    pool->tasks = (Task *)malloc(sizeof(Task) * capacity);
    pool->capacity = capacity;
    pool->front = 0;
    pool->rear = 0;
    pool->size = 0;

    pthread_mutex_init(&pool->lock, NULL);       
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);
}

void submit_task(ThreadPool *pool, Task task) {
    pthread_mutex_lock(&pool->lock);
    while (pool->size >= pool->capacity) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    pool->tasks[pool->front] = task;// 记录任务
    pool->front = (pool->front + 1) % pool->capacity;
    ++pool->size;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}

// 获得一个客户端连接
Task get_task(ThreadPool *pool) {
    pthread_mutex_lock(&pool->lock);
    Task task;
    while (pool->size == 0) {
        pthread_cond_wait(&pool->not_empty, &pool->lock);
    }

    task = pool->tasks[pool->rear];
    pool->rear = (pool->rear + 1) % pool->capacity;
    --pool->size;

    pthread_cond_signal(&pool->not_full);
    pthread_mutex_unlock(&pool->lock);
    return task;
}

void destroy_thread_pool(ThreadPool *pool) {
    free(pool->tasks);// 释放申请的内存
    pthread_mutex_destroy(&pool->lock);       
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
}

/*
#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);


#include <sys/types.h>
#include <sys/socket.h>

ssize_t send(int sockfd, const void *buf, size_t len, int flags);

*/
void *work_func(void * args) {
    ThreadPool *pool = (ThreadPool *)args;
    
    Task task = get_task(&pool);
    char get_strs[100];
    ssize_t bytes_read = recv(task->client_socket, get_strs, sizeof(get_strs));
    while (1) {
        if (bytes_read > 0) {
            printf("server receive message: %s\n", get_strs);
            char *response = "server receive your message";
            send(task->client_socket, response, sizeof(response), 0);
        } else if (bytes_read == 0) {
            printf("client close socket");
            break;
        } else {
            perror("recv failed");
            break;
        }
    }

    close(task->client_socket);
    return NULL;
}

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

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //如果需要与非本地地址客户端交互，需要用外部ip地址，不能用回环地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // 交互端口需要与客户端一致

    //2.绑定套接字
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //3.监听套接字
    if (listen(server_fd, 8) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("server listen on port: %d\n.........", PORT);
    
    // 创建线程池并初始化
    ThreadPool pool;
    init_thread_pool(&pool, DEFAULT_COUNT);
    pthread_t thread_ids[DEFAULT_COUNT];

    /*
        #include <pthread.h>

        int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
    */
    int i;
    for (i = 0; i < DEFAULT_COUNT; ++i) {
        pthread_create(&thread_ids[i], NULL, work_func, &pool);
    }
    /*
        #include <sys/types.h>
        #include <sys/socket.h>

        int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
        accept 函数返回一个新的文件描述符，这个文件描述符用于与客户端进行通信。如果出现错误，返回值为 -1
    */
    while (1) {
        //4.与客户端连接
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept failed");
            continue;
        }

        // 将客户端套接字封装为一个任务，交给线程池执行
        Task task = {client_socket};
        submit_task(&pool, task);
    }
    
    // 销毁线程池
    destroy_thread_pool(&pool);
    close(server_fd);
    return 0;
}