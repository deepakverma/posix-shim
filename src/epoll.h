// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <demi/libos.h>
#include <sys/epoll.h>

#define EPOLL_MAX_FDS 1024
#define EPOLL_MAX_FDS 1024
#define MAX_EVENTS 512

struct demi_event
{
    int used;
    int sockqd;
    demi_qtoken_t qt;
    demi_qresult_t qr;
    struct epoll_event ev;
};

extern void epoll_table_init(void);
extern int epoll_table_alloc(void);
extern struct demi_event *epoll_get_event(int epfd, int i);
extern __thread int __epoll_reent_guard;

#endif // _EPOLL_H_
