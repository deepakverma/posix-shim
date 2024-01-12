// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "../log.h"
#include "../qman.h"
#include <demi/libos.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
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

    // Check if this socket descriptor is managed by Demikernel.
    // If that is not the case, then fail to let the Linux kernel handle it.
    if (!queue_man_query_fd(sockfd))
    {
        errno = EBADF;
        return -1;
    }

    TRACE("sockfd=%d, addr=%p, addrlen=%d", sockfd, (void *)addr, addrlen);
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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
