// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "../log.h"
#include "../qman.h"
#include <demi/libos.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

/**
 * @brief Invokes demi_bind(), if the socket descriptor is managed by demikernel.
 *
 * @param sockfd  Socket descriptor.
 * @param addr    Socket address.
 * @param addrlen Effective size of socket address.
 *
 * @return If the socket descriptor is managed by Demikernel, then this function returns the result value of the
 * underlying Demikernel system call. Otherwise, this function returns -1 and sets errno to EBADF.
 */
int __demi_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = -1;



    TRACE("sockfd=%d, addr=%p, addrlen=%d", sockfd, (void *)addr, addrlen);
     if (addr->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
        char ipAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddr, INET_ADDRSTRLEN);
        TRACE("IPv4 Address: %s, Port: %d\n", ipAddr, ntohs(ipv4->sin_port));
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;
        char ipAddr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipAddr, INET6_ADDRSTRLEN);
        TRACE("IPv6 Address: %s, Port: %d\n", ipAddr, ntohs(ipv6->sin6_port));
    } else {
        TRACE("Unsupported address family: %d\n", addr->sa_family);
    }

    // Check if this socket descriptor is managed by Demikernel.
    // If that is not the case, then fail to let the Linux kernel handle it.
    if (!queue_man_query_fd(sockfd))
    {
        TRACE("fd=%d not managed by demikernel", sockfd);
        errno = EBADF;
        return -1;
    }
    struct in_addr ipAddress;

    inet_pton(AF_INET, "172.28.0.5", &ipAddress);

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = ipAddress.s_addr;
    server_address.sin_port = htons(6379);    
    
    // Invoke underlying Demikernel system call.

    if ((ret = demi_bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address))) != 0)
    {
        // The underlying Demikernel system call failed.
        errno = ret;
        if (ret == ENOSYS)
            errno = EBADF;
        ret = -1;
    }

    return (ret);
}
