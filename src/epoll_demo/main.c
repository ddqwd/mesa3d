
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define PORT 8080 

int main(int argc, char *argv[])
{
    int server_fd, client_fd; 

    struct sockaddr_in server_addr, client_addr;
    
     socklen_t addr_len = sizeof(struct sockaddr_in);
     struct epoll_event event, events[MAX_EVENTS];


    return 0;
}
