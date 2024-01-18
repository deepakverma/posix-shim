#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <assert.h>
#include "log.h"
#include <fcntl.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include "qman.h"
#include <string.h>


// Control-path hooks.
extern int __demi_init(void);
// Control-path hooks.
extern int __demi_init(void);
extern int __demi_socket(int domain, int type, int protocol);
extern int __demi_shutdown(int sockfd, int how);
extern int __demi_close(int sockfd);
extern int __demi_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern int __demi_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern int __demi_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int __demi_listen(int sockfd, int backlog);
extern int __demi_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int __demi_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int __demi_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
extern int __demi_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern int __demi_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// Data-path hooks.
extern ssize_t __demi_read(int sockfd, void *buf, size_t count);
extern ssize_t __demi_recv(int sockfd, void *buf, size_t len, int flags);
extern ssize_t __demi_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr,
                               socklen_t *addrlen);
extern ssize_t __demi_recvmsg(int sockfd, struct msghdr *msg, int flags);
extern ssize_t __demi_readv(int sockfd, const struct iovec *iov, int iovcnt);
extern ssize_t __demi_write(int sockfd, const void *buf, size_t count);
extern ssize_t __demi_send(int sockfd, const void *buf, size_t len, int flags);
extern ssize_t __demi_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr,
                             socklen_t addrlen);
extern ssize_t __demi_sendmsg(int sockfd, const struct msghdr *msg, int flags);
extern ssize_t __demi_writev(int sockfd, const struct iovec *iov, int iovcnt);
extern ssize_t __demi_pread(int sockfd, void *buf, size_t count, off_t offset);
extern ssize_t __demi_pwrite(int sockfd, const void *buf, size_t count, off_t offset);

// Epoll hooks
extern int __demi_epoll_create(int size);
extern int __demi_epoll_create1(int flags);
extern int __demi_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
extern int __demi_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

// Mutex for thread safety during initialization
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
// Flag to check if initialization is done
int is_initialized = 0;

// Function pointers for original system calls
int (*libc_socket)(int, int, int);
int (*libc_close)(int);
int (*libc_shutdown)(int, int);
int (*libc_bind)(int, const struct sockaddr *, socklen_t);
int (*libc_connect)(int, const struct sockaddr *, socklen_t);
int (*libc_listen)(int, int);
int (*libc_accept4)(int, struct sockaddr *, socklen_t *, int);
int (*libc_accept)(int, struct sockaddr *, socklen_t *);
int (*libc_getsockopt)(int, int, int, void *, socklen_t *);
int (*libc_setsockopt)(int, int, int, const void *, socklen_t);
int (*libc_getsockname)(int, struct sockaddr *, socklen_t *);
int (*libc_getpeername)(int, struct sockaddr *, socklen_t *);
ssize_t (*libc_read)(int, void *, size_t);
ssize_t (*libc_recv)(int, void *, size_t, int);
ssize_t (*libc_recvfrom)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t (*libc_recvmsg)(int, struct msghdr *, int);
ssize_t (*libc_readv)(int, const struct iovec *, int);
ssize_t (*libc_pread)(int, void *, size_t, off_t);
ssize_t (*libc_write)(int, const void *, size_t);
ssize_t (*libc_send)(int, const void *, size_t, int);
ssize_t (*libc_sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
ssize_t (*libc_sendmsg)(int, const struct msghdr *, int);
ssize_t (*libc_writev)(int, const struct iovec *, int);
ssize_t (*libc_pwrite)(int, const void *, size_t, off_t);
int (*libc_epoll_create)(int);
int (*libc_epoll_create1)(int);
int (*libc_epoll_ctl)(int, int, int, struct epoll_event *);
int (*libc_epoll_wait)(int, struct epoll_event *, int, int);
int (*libc_fcntl)(int fd, int cmd, ...);

__thread int reentrant_init = 0;
// Initialization function with mutex
void init_functions() {
    if (!is_initialized)
    {
        if(reentrant_init) return;
        pthread_mutex_lock(&init_mutex);
        TRACE("init");
        if (!is_initialized) {
            assert((libc_socket = dlsym(RTLD_NEXT, "socket")) !=NULL);
            assert((libc_close = dlsym(RTLD_NEXT, "close")) != NULL);
            assert((libc_shutdown = dlsym(RTLD_NEXT, "shutdown"))  != NULL);
            assert((libc_epoll_create1 = dlsym(RTLD_NEXT, "epoll_create1"))  != NULL);
            assert((libc_epoll_create = dlsym(RTLD_NEXT, "epoll_create"))  != NULL);
            assert((libc_epoll_ctl = dlsym(RTLD_NEXT, "epoll_ctl"))  != NULL);
            assert((libc_read = dlsym(RTLD_NEXT, "read"))  != NULL);
            assert((libc_write = dlsym(RTLD_NEXT, "write"))  != NULL);
            assert((libc_bind = dlsym(RTLD_NEXT, "bind")) != NULL);
            assert((libc_listen = dlsym(RTLD_NEXT, "listen")) != NULL);
            assert((libc_fcntl = dlsym(RTLD_NEXT, "fcntl")) != NULL);
            assert((libc_epoll_wait = dlsym(RTLD_NEXT, "epoll_wait")) != NULL);
            assert((libc_accept = dlsym(RTLD_NEXT, "accept")) != NULL);
            reentrant_init = 1;
            __demi_init();
            reentrant_init = 0;
            is_initialized = 1;
        }

        pthread_mutex_unlock(&init_mutex);
    }
}

// Override system calls
int socket(int domain, int type, int protocol) {
    init_functions();
    assert(libc_socket != NULL);
    if(domain == AF_INET6) return -1; // force libevent to create IPv4
    return libc_socket(domain, type, protocol);
}

int fcntl(int fd, int cmd, ... /* arg */) {
    TRACE("fcntl %d", fd);
    int demifd;
    if((demifd = queue_man_query_fd_demifd(fd)) != -1)
    {
        return 1;
    }
    init_functions();
    va_list args;
    va_start(args, cmd);
    int result = libc_fcntl(fd, cmd, va_arg(args, long));  // Assuming the argument is 'long'
    va_end(args);
    return result;
}


// int epoll_create1(int flag) {
//     init_functions();
//     assert(libc_epoll_create1 != NULL);
//     return libc_epoll_create1(flag);
// }

// int epoll_create(int size) {
//     init_functions();
//     assert(libc_epoll_create != NULL);
//     return libc_epoll_create(size);
// }

__thread int reentrant = 0;
// __thread int epoll_ctl_reentrant = 0;
// __thread int accept_reentrant = 0;
// __thread int read_reentrant = 0;

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    init_functions();
    assert(libc_epoll_ctl != NULL);
    if(!reentrant) {
        int demifd;
        if((demifd = queue_man_query_fd_demifd(fd)) != -1)
        {
            // register the fd with demiepoll
            int demiepollfd = queue_man_get_demikernel_epfd(demifd);
            TRACE("epoll_ctl detected demikernel fd, epfd=%d sockfd=%d, demifd=%d demiepollfd=%d", epfd, fd, demifd, demiepollfd);
            reentrant= 1;
            int ret = __demi_epoll_ctl(demiepollfd, op, demifd, event);
            reentrant = 0;
            // store mapping of epfd to demikernel epfd
            queue_man_link_fd_demiepollfd(epfd, demiepollfd);
            TRACE("epoll_ctl ret=%d", ret);
            return ret;
        }
    }
    return libc_epoll_ctl(epfd, op, fd, event);
}

//__thread int epoll_wait_reentrant = 0;
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout){
    init_functions();
    assert(libc_epoll_wait != NULL);
    int demiepfd;
    if(!reentrant) {
        if((demiepfd = queue_man_query_fd_demiepollfd(epfd)) != -1)
        {
            TRACE("detected demikernel, waiting demiepdf=%d", demiepfd);
            reentrant = 1;
            __demi_epoll_wait(demiepfd, events, maxevents, timeout);
            reentrant = 0;
        }
    }
    return libc_epoll_wait(epfd, events, maxevents, timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    TRACE("Accept sockfd=%d reentrant=%d", sockfd, reentrant);
    init_functions();
    assert(libc_accept != NULL);
    if(!reentrant) {
        int demifd;
        if((demifd = queue_man_query_fd_demifd(sockfd)) != -1) {
            reentrant = 1;
            __demi_accept(demifd, addr, addrlen);
            reentrant = 0;
        }
    }
    return libc_accept(sockfd, addr, addrlen);
}

int read(int fd, void *buf, size_t count){
    TRACE("read fd=%d", fd);
    init_functions();
    assert(libc_read != NULL);
    if(!reentrant) {
        int demifd;
        if((demifd = queue_man_query_fd_demifd(fd)) != -1)
        {
            reentrant = 1;
            int ret = __demi_read(demifd, buf, count);
            reentrant = 0;
            return ret;
        }
    }
    TRACE("read completed");
    return libc_read(fd, buf, count);
}

//__thread int write_reentrant = 0;
int write(int fd, void *buf, size_t count){
    init_functions();
    assert(libc_write != NULL);
    if(!reentrant) {
        int demifd;
        if((demifd = queue_man_query_fd_demifd(fd)) != -1)
        {
            reentrant = 1;
            int ret = __demi_write(demifd, buf, count);
            reentrant = 0;
            return ret;
        }
    }
    return libc_write(fd, buf, count);
}

//__thread int bindreentrant = 0;
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    TRACE("bind%d %d", reentrant, sockfd);
    init_functions();

    if(!reentrant) {
        assert(libc_bind != NULL);
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
        char ipAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddr, INET_ADDRSTRLEN);
        TRACE("IPv4 Address: %s, Port: %d\n", ipAddr, ntohs(ipv4->sin_port));

        int port = 15476;
        // hack to create demikernel socket mapping
        if(ntohs(ipv4->sin_port) == port)
        {
            TRACE("detected port");
            int demifd = __demi_socket(AF_INET, SOCK_STREAM, 0);
            TRACE("created demikernel socket demi=%d libc=%d",demifd, sockfd);
            queue_man_register_fd(demifd);

            // override address as demikernel cannot bind to wildcard address
            struct sockaddr_in local_addr;
            memset(&local_addr, 0, sizeof(local_addr));
            local_addr.sin_family = AF_INET;
            local_addr.sin_addr.s_addr = inet_addr("172.28.0.5");  // Specific local IPv4 address
            local_addr.sin_port = htons(port);

            // bind demi socket
            reentrant = 1;
            int ret = __demi_bind(demifd,(struct sockaddr *) &local_addr, sizeof(local_addr));
            // create demi epoll
            int demiepoll = __demi_epoll_create(MAX_EVENTS);
            reentrant = 0;

            queue_man_link_fd_demifd(sockfd, demifd);
            // mapping of demifd todemiepoll
            queue_man_register_linux_epfd(demifd, demiepoll);

            return ret;
        }
    } else {
        TRACE("reentrantbind with sockfd =%d", sockfd);
    }
    TRACE("bind completed");
    return libc_bind(sockfd, addr, addrlen);
}

int listen(int sockfd, int backlog) {
    TRACE("listen");
    init_functions();
    assert(libc_listen != NULL);
    if(!reentrant) {
        int demifd;
        if((demifd = queue_man_query_fd_demifd(sockfd)) != -1)
        {
            TRACE("demifd %d", demifd);
            reentrant = 1;
            int ret = __demi_listen(demifd, backlog);
            reentrant = 0;
            return ret;
        }
    }
    TRACE("listen completed");
    return libc_listen(sockfd, backlog);
}

int close(int fd) {
    init_functions();
    assert(libc_close != NULL);
    return libc_close(fd);
}
