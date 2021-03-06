/**
    socket.c - Convenience class for the management of sockets

    This module provides a higher interface to interact with the standard sockets API. It does not perform buffering.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

#if !VXWORKS && !WINCE
/*
    On MAC OS X, getaddrinfo is not thread-safe and crashes when called by a 2nd thread at any time. ie. locking wont help.
 */
#define BIT_HAS_GETADDRINFO 1
#endif

/********************************** Defines ***********************************/

#ifndef BIT_MAX_IP
    #define BIT_MAX_IP 1024
#endif

/********************************** Forwards **********************************/

static void closeSocket(MprSocket *sp, bool gracefully);
static int connectSocket(MprSocket *sp, cchar *ipAddr, int port, int initialFlags);
static MprSocketProvider *createStandardProvider(MprSocketService *ss);
static void disconnectSocket(MprSocket *sp);
static ssize flushSocket(MprSocket *sp);
static int getSocketIpAddr(struct sockaddr *addr, int addrlen, char *ip, int size, int *port);
static int ipv6(cchar *ip);
static void manageSocket(MprSocket *sp, int flags);
static void manageSocketService(MprSocketService *ss, int flags);
static void manageSsl(MprSsl *ssl, int flags);
static ssize readSocket(MprSocket *sp, void *buf, ssize bufsize);
static char *socketState(MprSocket *sp);
static ssize writeSocket(MprSocket *sp, cvoid *buf, ssize bufsize);

/************************************ Code ************************************/
/*
    Open the socket service
 */

PUBLIC MprSocketService *mprCreateSocketService()
{
    MprSocketService    *ss;
    char                hostName[BIT_MAX_IP], serverName[BIT_MAX_IP], domainName[BIT_MAX_IP], *dp;
    Socket              fd;

    if ((ss = mprAllocObj(MprSocketService, manageSocketService)) == 0) {
        return 0;
    }
    ss->maxAccept = MAXINT;
    ss->numAccept = 0;

    if ((ss->standardProvider = createStandardProvider(ss)) == 0) {
        return 0;
    }
    if ((ss->mutex = mprCreateLock()) == 0) {
        return 0;
    }
    serverName[0] = '\0';
    domainName[0] = '\0';
    hostName[0] = '\0';
    if (gethostname(serverName, sizeof(serverName)) < 0) {
        scopy(serverName, sizeof(serverName), "localhost");
        mprError("Cannot get host name. Using \"localhost\".");
        /* Keep going */
    }
    if ((dp = strchr(serverName, '.')) != 0) {
        scopy(hostName, sizeof(hostName), serverName);
        *dp++ = '\0';
        scopy(domainName, sizeof(domainName), dp);

    } else {
        scopy(hostName, sizeof(hostName), serverName);
    }
    mprSetServerName(serverName);
    mprSetDomainName(domainName);
    mprSetHostName(hostName);
    ss->secureSockets = mprCreateList(0, 0);

    if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) != -1) {
        ss->hasIPv6 = 1;
        closesocket(fd);
    } else {
        mprLog(1, "This system does not have IPv6 support");
    }
    return ss;
}


static void manageSocketService(MprSocketService *ss, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ss->standardProvider);
        mprMark(ss->providers);
        mprMark(ss->sslProvider);
        mprMark(ss->mutex);
        mprMark(ss->secureSockets);
    }
}


static void manageSocketProvider(MprSocketProvider *provider, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(provider->name);
    }
}


static MprSocketProvider *createStandardProvider(MprSocketService *ss)
{
    MprSocketProvider   *provider;

    if ((provider = mprAllocObj(MprSocketProvider, manageSocketProvider)) == 0) {
        return 0;
    }
    provider->name = sclone("standard");;
    provider->closeSocket = closeSocket;
    provider->disconnectSocket = disconnectSocket;
    provider->flushSocket = flushSocket;
    provider->readSocket = readSocket;
    provider->writeSocket = writeSocket;
    provider->socketState = socketState;
    return provider;
}


PUBLIC void mprAddSocketProvider(cchar *name, MprSocketProvider *provider)
{
    MprSocketService    *ss;

    ss = MPR->socketService;

    if (ss->providers == 0 && (ss->providers = mprCreateHash(0, 0)) == 0) {
        return;
    }
    provider->name = sclone(name);
    mprAddKey(ss->providers, name, provider);
}


PUBLIC bool mprHasSecureSockets()
{
    return (MPR->socketService->providers != 0);
}


PUBLIC int mprSetMaxSocketAccept(int max)
{
    assert(max >= 0);

    MPR->socketService->maxAccept = max;
    return 0;
}


PUBLIC MprSocket *mprCreateSocket()
{
    MprSocketService    *ss;
    MprSocket           *sp;

    ss = MPR->socketService;
    if ((sp = mprAllocObj(MprSocket, manageSocket)) == 0) {
        return 0;
    }
    sp->port = -1;
    sp->fd = INVALID_SOCKET;

    sp->provider = ss->standardProvider;
    sp->service = ss;
    sp->mutex = mprCreateLock();
    return sp;
}


static void manageSocket(MprSocket *sp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(sp->service);
        mprMark(sp->errorMsg);
        mprMark(sp->handler);
        mprMark(sp->acceptIp);
        mprMark(sp->ip);
        mprMark(sp->provider);
        mprMark(sp->listenSock);
        mprMark(sp->sslSocket);
        mprMark(sp->ssl);
        mprMark(sp->mutex);

    } else if (flags & MPR_MANAGE_FREE) {
        if (sp->fd != INVALID_SOCKET) {
            sp->mutex = 0;
            mprCloseSocket(sp, 1);
        }
    }
}


/*
    Re-initialize all socket variables so the socket can be reused. This closes the socket and removes all wait handlers.
 */
static void resetSocket(MprSocket *sp)
{
    if (sp->fd != INVALID_SOCKET) {
        mprCloseSocket(sp, 0);
    }
    if (sp->flags & MPR_SOCKET_CLOSED) {
        sp->flags = 0;
        sp->port = -1;
        sp->fd = INVALID_SOCKET;
        sp->ip = 0;
    }
    assert(sp->provider);
}


PUBLIC bool mprHasDualNetworkStack() 
{
    bool dual;

#if defined(BIT_HAS_SINGLE_STACK) || VXWORKS
    dual = 0;
#elif WINDOWS
    {
        OSVERSIONINFO info;
        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionEx(&info);
        /* Vista or later */
        dual = info.dwMajorVersion >= 6;
    }
#else
    dual = MPR->socketService->hasIPv6;
#endif
    return dual;
}


PUBLIC bool mprHasIPv6() 
{
    return MPR->socketService->hasIPv6;
}


/*
    Open a server connection
 */
PUBLIC Socket mprListenOnSocket(MprSocket *sp, cchar *ip, int port, int flags)
{
    struct sockaddr     *addr;
    Socklen             addrlen;
    cchar               *sip;
    int                 datagram, family, protocol, rc, only;

    lock(sp);
    if (ip == 0 || *ip == '\0') {
        mprTrace(6, "mprListenOnSocket: %d, flags %x", port, flags);
    } else {
        mprTrace(6, "mprListenOnSocket: %s:%d, flags %x", ip, port, flags);
    }
    resetSocket(sp);

    sp->ip = sclone(ip);
    sp->fd = INVALID_SOCKET;
    sp->port = port;
    sp->flags = (flags & (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK |
         MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY | MPR_SOCKET_THREAD));
    datagram = sp->flags & MPR_SOCKET_DATAGRAM;

    /*
        Change null IP address to be an IPv6 endpoint if the system is dual-stack. That way we can listen on 
        both IPv4 and IPv6
     */
    sip = ((ip == 0 || *ip == '\0') && mprHasDualNetworkStack()) ? "::" : ip;

    if (mprGetSocketInfo(sip, port, &family, &protocol, &addr, &addrlen) < 0) {
        unlock(sp);
        return SOCKET_ERROR;
    }
    if ((sp->fd = (int) socket(family, datagram ? SOCK_DGRAM: SOCK_STREAM, protocol)) == SOCKET_ERROR) {
        unlock(sp);
        assert(sp->fd == INVALID_SOCKET);
        return SOCKET_ERROR;
    }

#if !BIT_WIN_LIKE && !VXWORKS
    /*
        Children won't inherit this fd
     */
    fcntl(sp->fd, F_SETFD, FD_CLOEXEC);
#endif

    if (!(sp->flags & MPR_SOCKET_NOREUSE)) {
        rc = 1;
#if BIT_UNIX_LIKE || VXWORKS
        setsockopt(sp->fd, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
#elif BIT_WIN_LIKE && defined(SO_EXCLUSIVEADDRUSE)
        setsockopt(sp->fd, SOL_SOCKET, SO_REUSEADDR | SO_EXCLUSIVEADDRUSE, (char*) &rc, sizeof(rc));
#endif
    }
    /*
        By default, most stacks listen on both IPv6 and IPv4 if ip == 0, except windows which inverts this.
        So we explicitly control.
     */
#if defined(IPV6_V6ONLY)
    if (MPR->socketService->hasIPv6) {
        if (ip == 0 || *ip == '\0') {
            only = 0;
            setsockopt(sp->fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &only, sizeof(only));
        } else if (ipv6(ip)) {
            only = 1;
            setsockopt(sp->fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &only, sizeof(only));
        }
    }
#endif
    if (sp->service->prebind) {
        if ((sp->service->prebind)(sp) < 0) {
            closesocket(sp->fd);
            sp->fd = INVALID_SOCKET;
            unlock(sp);
            return SOCKET_ERROR;
        }
    }
    if ((rc = bind(sp->fd, addr, addrlen)) < 0) {
        if (errno == EADDRINUSE) {
            mprLog(3, "Cannot bind, address %s:%d already in use", ip, port);
        } else {
            mprLog(3, "Cannot bind, address %s:%d errno", ip, port, errno);
        }
        rc = mprGetOsError();
        closesocket(sp->fd);
        mprSetOsError(rc);
        sp->fd = INVALID_SOCKET;
        unlock(sp);
        return SOCKET_ERROR;
    }

    /* NOTE: Datagrams have not been used in a long while. Maybe broken */
    if (!datagram) {
        sp->flags |= MPR_SOCKET_LISTENER;
        if (listen(sp->fd, SOMAXCONN) < 0) {
            mprLog(3, "Listen error %d", mprGetOsError());
            closesocket(sp->fd);
            sp->fd = INVALID_SOCKET;
            unlock(sp);
            return SOCKET_ERROR;
        }
    }

#if BIT_WIN_LIKE
    /*
        Delay setting reuse until now so that we can be assured that we have exclusive use of the port.
     */
    if (!(sp->flags & MPR_SOCKET_NOREUSE)) {
        int rc = 1;
        setsockopt(sp->fd, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
    }
#endif
    mprSetSocketBlockingMode(sp, (bool) (sp->flags & MPR_SOCKET_BLOCK));

    /*
        TCP/IP stacks have the No delay option (nagle algorithm) on by default.
     */
    if (sp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(sp, 1);
    }
    unlock(sp);
    return sp->fd;
}


PUBLIC MprWaitHandler *mprAddSocketHandler(MprSocket *sp, int mask, MprDispatcher *dispatcher, void *proc, 
    void *data, int flags)
{
    assert(sp);
    assert(sp->fd != INVALID_SOCKET);
    assert(proc);

    if (sp->fd == INVALID_SOCKET) {
        return 0;
    }
    if (sp->handler) {
        mprRemoveWaitHandler(sp->handler);
    }
    if (sp->flags & MPR_SOCKET_BUFFERED_READ) {
        mask |= MPR_READABLE;
    }
    if (sp->flags & MPR_SOCKET_BUFFERED_WRITE) {
        mask |= MPR_WRITABLE;
    }
    sp->handler = mprCreateWaitHandler((int) sp->fd, mask, dispatcher, proc, data, flags);
    return sp->handler;
}


PUBLIC void mprRemoveSocketHandler(MprSocket *sp)
{
    if (sp->handler) {
        mprRemoveWaitHandler(sp->handler);
        sp->handler = 0;
    }
}


PUBLIC void mprSetSocketDispatcher(MprSocket *sp, MprDispatcher *dispatcher)
{
    if (sp->handler) {
        sp->handler->dispatcher = dispatcher;
    }
}


PUBLIC void mprHiddenSocketData(MprSocket *sp, ssize len, int dir)
{
    lock(sp);
    if (len > 0) {
        sp->flags |= (dir == MPR_READABLE) ? MPR_SOCKET_BUFFERED_READ : MPR_SOCKET_BUFFERED_WRITE;
        if (sp->handler) {
            mprRecallWaitHandler(sp->handler);
        }
    } else {
        sp->flags &= ~((dir == MPR_READABLE) ? MPR_SOCKET_BUFFERED_READ : MPR_SOCKET_BUFFERED_WRITE);
    }
    unlock(sp);
}


//  TODO rename to mprWaitOnSocket

PUBLIC void mprEnableSocketEvents(MprSocket *sp, int mask)
{
    assert(sp->handler);
    if (sp->handler) {
        if (sp->flags & MPR_SOCKET_BUFFERED_READ) {
            mask |= MPR_READABLE;
        }
        if (sp->flags & MPR_SOCKET_BUFFERED_WRITE) {
            mask |= MPR_WRITABLE;
        }
        if (sp->flags & (MPR_SOCKET_BUFFERED_READ | MPR_SOCKET_BUFFERED_WRITE)) {
            if (sp->handler) {
                mprRecallWaitHandler(sp->handler);
            }
        }
        mprWaitOn(sp->handler, mask);
    }
}


/*
    Open a client socket connection
 */
PUBLIC int mprConnectSocket(MprSocket *sp, cchar *ip, int port, int flags)
{
    if (sp->provider == 0) {
        return MPR_ERR_NOT_INITIALIZED;
    }
    return connectSocket(sp, ip, port, flags);
}


static int connectSocket(MprSocket *sp, cchar *ip, int port, int initialFlags)
{
    struct sockaddr     *addr;
    Socklen             addrlen;
    int                 broadcast, datagram, family, protocol, rc;

    mprTrace(6, "openClient: %s:%d, flags %x", ip, port, initialFlags);

    lock(sp);
    resetSocket(sp);

    sp->port = port;
    sp->flags = (initialFlags &
        (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK |
         MPR_SOCKET_LISTENER | MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY | MPR_SOCKET_THREAD));
    sp->ip = sclone(ip);

    broadcast = sp->flags & MPR_SOCKET_BROADCAST;
    if (broadcast) {
        sp->flags |= MPR_SOCKET_DATAGRAM;
    }
    datagram = sp->flags & MPR_SOCKET_DATAGRAM;

    if (mprGetSocketInfo(ip, port, &family, &protocol, &addr, &addrlen) < 0) {
        closesocket(sp->fd);
        sp->fd = INVALID_SOCKET;
        unlock(sp);
        return MPR_ERR_CANT_ACCESS;
    }
    if ((sp->fd = (int) socket(family, datagram ? SOCK_DGRAM: SOCK_STREAM, protocol)) < 0) {
        unlock(sp);
        return MPR_ERR_CANT_OPEN;
    }
#if !BIT_WIN_LIKE && !VXWORKS
    /*
        Children should not inherit this fd
     */
    fcntl(sp->fd, F_SETFD, FD_CLOEXEC);
#endif
    if (broadcast) {
        int flag = 1;
        if (setsockopt(sp->fd, SOL_SOCKET, SO_BROADCAST, (char *) &flag, sizeof(flag)) < 0) {
            closesocket(sp->fd);
            sp->fd = INVALID_SOCKET;
            unlock(sp);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    if (!datagram) {
        sp->flags |= MPR_SOCKET_CONNECTING;
        do {
            rc = connect(sp->fd, addr, addrlen);
        } while (rc == -1 && errno == EINTR);
        if (rc < 0) {
            /* MAC/BSD returns EADDRINUSE */
            if (errno == EINPROGRESS || errno == EALREADY || errno == EADDRINUSE) {
#if BIT_UNIX_LIKE
                do {
                    struct pollfd pfd;
                    pfd.fd = sp->fd;
                    pfd.events = POLLOUT;
                    rc = poll(&pfd, 1, 1000);
                } while (rc < 0 && errno == EINTR);
#endif
                if (rc > 0) {
                    errno = EISCONN;
                }
            } 
            if (errno != EISCONN) {
                closesocket(sp->fd);
                sp->fd = INVALID_SOCKET;
                unlock(sp);
                return MPR_ERR_CANT_COMPLETE;
            }
        }
    }
    mprSetSocketBlockingMode(sp, (bool) (sp->flags & MPR_SOCKET_BLOCK));

    /*
        TCP/IP stacks have the no delay option (nagle algorithm) on by default.
     */
    if (sp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(sp, 1);
    }
    unlock(sp);
    return 0;
}


/*
    Abortive disconnect. Thread-safe. (e.g. from a timeout or callback thread). This closes the underlying socket file
    descriptor but keeps the handler and socket object intact. It also forces a recall on the wait handler.
 */
PUBLIC void mprDisconnectSocket(MprSocket *sp)
{
    if (sp && sp->provider) {
        sp->provider->disconnectSocket(sp);
    }
}


static void disconnectSocket(MprSocket *sp)
{
    char    buf[BIT_MAX_BUFFER];
    int     i;

    /*
        Defensive lock buster. Use try lock incase an operation is blocked somewhere with a lock asserted. 
        Should never happen.
     */
    if (!mprTryLock(sp->mutex)) {
        return;
    }
    if (sp->fd == INVALID_SOCKET || !(sp->flags & MPR_SOCKET_EOF)) {
        /*
            Read a reasonable amount of outstanding data to minimize resets. Then do a shutdown to send a FIN and read 
            outstanding data.  All non-blocking.
         */
        mprTrace(6, "Disconnect socket %d", sp->fd);
        mprSetSocketBlockingMode(sp, 0);
        for (i = 0; i < 16; i++) {
            if (recv(sp->fd, buf, sizeof(buf), 0) <= 0) {
                break;
            }
        }
        shutdown(sp->fd, SHUT_RDWR);
        for (i = 0; i < 16; i++) {
            if (recv(sp->fd, buf, sizeof(buf), 0) <= 0) {
                break;
            }
        }
        sp->flags |= MPR_SOCKET_EOF | MPR_SOCKET_DISCONNECTED;
        if (sp->handler) {
            mprRecallWaitHandler(sp->handler);
        }
    }
    unlock(sp);
}


PUBLIC void mprCloseSocket(MprSocket *sp, bool gracefully)
{
    if (sp == NULL) {
        return;
    }
    assert(sp->provider);
    if (sp->provider == 0) {
        return;
    }
    mprRemoveSocketHandler(sp);
    sp->provider->closeSocket(sp, gracefully);
}


/*
    Standard (non-SSL) close. Permit multiple calls.
 */
static void closeSocket(MprSocket *sp, bool gracefully)
{
    MprSocketService    *ss;
    MprTime             timesUp;
    char                buf[16];

    ss = MPR->socketService;

    lock(sp);
    if (sp->flags & MPR_SOCKET_CLOSED) {
        unlock(sp);
        return;
    }
    sp->flags |= MPR_SOCKET_CLOSED | MPR_SOCKET_EOF;

    if (sp->fd != INVALID_SOCKET) {
        /*
            Read any outstanding read data to minimize resets. Then do a shutdown to send a FIN and read outstanding 
            data. All non-blocking.
         */
        mprTrace(6, "Close socket %d, graceful %d", sp->fd, gracefully);
        if (gracefully) {
            mprSetSocketBlockingMode(sp, 0);
            while (recv(sp->fd, buf, sizeof(buf), 0) > 0) { }
        }
        if (shutdown(sp->fd, SHUT_RDWR) == 0) {
            if (gracefully) {
                timesUp = mprGetTime() + MPR_TIMEOUT_LINGER;
                do {
                    if (recv(sp->fd, buf, sizeof(buf), 0) <= 0) {
                        break;
                    }
                } while (mprGetTime() < timesUp);
            }
        }
        closesocket(sp->fd);
        sp->fd = INVALID_SOCKET;
    }

    if (sp->flags & MPR_SOCKET_SERVER) {
        lock(ss);
        if (--ss->numAccept < 0) {
            ss->numAccept = 0;
        }
        unlock(ss);
    }
    unlock(sp);
}


PUBLIC MprSocket *mprAcceptSocket(MprSocket *listen)
{
    MprSocketService            *ss;
    MprSocket                   *nsp;
    struct sockaddr_storage     addrStorage, saddrStorage;
    struct sockaddr             *addr, *saddr;
    char                        ip[BIT_MAX_IP], acceptIp[BIT_MAX_IP];
    Socklen                     addrlen, saddrlen;
    Socket                      fd;
    int                         port, acceptPort;

    ss = MPR->socketService;
    addr = (struct sockaddr*) &addrStorage;
    addrlen = sizeof(addrStorage);

    if (listen->flags & MPR_SOCKET_BLOCK) {
        mprYield(MPR_YIELD_STICKY);
    }
    fd = accept(listen->fd, addr, &addrlen);
    if (listen->flags & MPR_SOCKET_BLOCK) {
        mprResetYield();
    }
    if (fd == SOCKET_ERROR) {
        if (mprGetError() != EAGAIN) {
            mprTrace(6, "socket: accept failed, errno %d", mprGetOsError());
        }
        return 0;
    }
    if ((nsp = mprCreateSocket()) == 0) {
        closesocket(fd);
        return 0;
    }
    nsp->fd = fd;
    nsp->listenSock = listen;
    nsp->port = listen->port;
    nsp->flags = ((listen->flags & ~MPR_SOCKET_LISTENER) | MPR_SOCKET_SERVER);

    /*
        Limit the number of simultaneous clients
     */
    lock(ss);
    if (++ss->numAccept >= ss->maxAccept) {
        unlock(ss);
        mprLog(2, "Rejecting connection, too many client connections (%d)", ss->numAccept);
        mprCloseSocket(nsp, 0);
        return 0;
    }
    unlock(ss);

#if !BIT_WIN_LIKE && !VXWORKS
    /* Prevent children inheriting this socket */
    fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

    mprSetSocketBlockingMode(nsp, (nsp->flags & MPR_SOCKET_BLOCK) ? 1: 0);
    if (nsp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(nsp, 1);
    }
    /*
        Get the remote client address
     */
    if (getSocketIpAddr(addr, addrlen, ip, sizeof(ip), &port) != 0) {
        assert(0);
        mprCloseSocket(nsp, 0);
        return 0;
    }
    nsp->ip = sclone(ip);
    nsp->port = port;

    /*
        Get the server interface address accepting the connection
     */
    saddr = (struct sockaddr*) &saddrStorage;
    saddrlen = sizeof(saddrStorage);
    getsockname(fd, saddr, &saddrlen);
    acceptPort = 0;
    getSocketIpAddr(saddr, saddrlen, acceptIp, sizeof(acceptIp), &acceptPort);
    nsp->acceptIp = sclone(acceptIp);
    nsp->acceptPort = acceptPort;
    return nsp;
}


/*
    Read data. Return -1 for EOF and errors. On success, return the number of bytes read.
 */
PUBLIC ssize mprReadSocket(MprSocket *sp, void *buf, ssize bufsize)
{
    assert(sp);
    assert(buf);
    assert(bufsize > 0);
    assert(sp->provider);

    if (sp->provider == 0) {
        return MPR_ERR_NOT_INITIALIZED;
    }
    return sp->provider->readSocket(sp, buf, bufsize);
}


/*
    Standard read from a socket (Non SSL)
    Return number of bytes read. Return -1 on errors and EOF.
 */
static ssize readSocket(MprSocket *sp, void *buf, ssize bufsize)
{
    struct sockaddr_storage server;
    Socklen                 len;
    ssize                   bytes;
    int                     errCode;

    assert(buf);
    assert(bufsize > 0);
    assert(~(sp->flags & MPR_SOCKET_CLOSED));

    lock(sp);
    if (sp->flags & MPR_SOCKET_EOF) {
        unlock(sp);
        return -1;
    }
again:
    if (sp->flags & MPR_SOCKET_BLOCK) {
        mprYield(MPR_YIELD_STICKY);
    }
    if (sp->flags & MPR_SOCKET_DATAGRAM) {
        len = sizeof(server);
        bytes = recvfrom(sp->fd, buf, (int) bufsize, MSG_NOSIGNAL, (struct sockaddr*) &server, (Socklen*) &len);
    } else {
        bytes = recv(sp->fd, buf, (int) bufsize, MSG_NOSIGNAL);
    }
    if (sp->flags & MPR_SOCKET_BLOCK) {
        mprResetYield();
    }
    if (bytes < 0) {
        errCode = mprGetSocketError(sp);
        if (errCode == EINTR) {
            goto again;

        } else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
            bytes = 0;                          /* No data available */

        } else if (errCode == ECONNRESET) {
            sp->flags |= MPR_SOCKET_EOF;        /* Disorderly disconnect */
            bytes = -1;

        } else {
            sp->flags |= MPR_SOCKET_EOF;        /* Some other error */
            bytes = -errCode;
        }

    } else if (bytes == 0) {                    /* EOF */
        sp->flags |= MPR_SOCKET_EOF;
        bytes = -1;
    }
    unlock(sp);
    return bytes;
}


/*
    Write data. Return the number of bytes written or -1 on errors. NOTE: this routine will return with a
    short write if the underlying socket can't accept any more data.
 */
PUBLIC ssize mprWriteSocket(MprSocket *sp, cvoid *buf, ssize bufsize)
{
    assert(sp);
    assert(buf);
    assert(bufsize > 0);
    assert(sp->provider);

    if (sp->provider == 0) {
        return MPR_ERR_NOT_INITIALIZED;
    }
    return sp->provider->writeSocket(sp, buf, bufsize);
}


/*
    Standard write to a socket (Non SSL)
    Return count of bytes written. mprGetError will return EAGAIN or EWOULDBLOCK if transport is saturated.
 */
static ssize writeSocket(MprSocket *sp, cvoid *buf, ssize bufsize)
{
    struct sockaddr     *addr;
    Socklen             addrlen;
    ssize               len, written, sofar;
    int                 family, protocol, errCode;

    assert(buf);
    assert(bufsize >= 0);
    assert((sp->flags & MPR_SOCKET_CLOSED) == 0);

    lock(sp);
    if (sp->flags & (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM)) {
        if (mprGetSocketInfo(sp->ip, sp->port, &family, &protocol, &addr, &addrlen) < 0) {
            unlock(sp);
            return MPR_ERR_CANT_FIND;
        }
    }
    if (sp->flags & MPR_SOCKET_EOF) {
        sofar = MPR_ERR_CANT_WRITE;
    } else {
        errCode = 0;
        len = bufsize;
        sofar = 0;
        while (len > 0) {
            unlock(sp);
            if (sp->flags & MPR_SOCKET_BLOCK) {
                mprYield(MPR_YIELD_STICKY);
            }
            if ((sp->flags & MPR_SOCKET_BROADCAST) || (sp->flags & MPR_SOCKET_DATAGRAM)) {
                written = sendto(sp->fd, &((char*) buf)[sofar], (int) len, MSG_NOSIGNAL, addr, addrlen);
            } else {
                written = send(sp->fd, &((char*) buf)[sofar], (int) len, MSG_NOSIGNAL);
            }
            /* Get the error code before calling mprResetYield to avoid clearing global error numbers */
            errCode = mprGetSocketError(sp);
            if (sp->flags & MPR_SOCKET_BLOCK) {
                mprResetYield();
            }
            lock(sp);
            if (written < 0) {
                assert(errCode != 0);
                if (errCode == EINTR) {
                    continue;
                } else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
#if BIT_WIN_LIKE
                    /*
                        Windows sockets don't support blocking I/O. So we simulate here
                        OPT - could wait for a writable event
                     */
                    if (sp->flags & MPR_SOCKET_BLOCK) {
                        mprNap(0);
                        continue;
                    }
#endif
                    unlock(sp);
                    if (sofar) {
                        return sofar;
                    }
                    return -errCode;
                }
                unlock(sp);
                return -errCode;
            }
            len -= written;
            sofar += written;
        }
    }
    unlock(sp);
    return sofar;
}


/*
    Write a string to the socket
 */
PUBLIC ssize mprWriteSocketString(MprSocket *sp, cchar *str)
{
    return mprWriteSocket(sp, str, slen(str));
}


PUBLIC ssize mprWriteSocketVector(MprSocket *sp, MprIOVec *iovec, int count)
{
    char        *start;
    ssize       total, len, written;
    int         i;

#if BIT_UNIX_LIKE
    if (sp->sslSocket == 0) {
        return writev(sp->fd, (const struct iovec*) iovec, (int) count);
    } else
#endif
    {
        //  OPT - better to buffer and have fewer raw writes
        if (count <= 0) {
            return 0;
        }
        start = iovec[0].start;
        len = (int) iovec[0].len;
        assert(len > 0);

        for (total = i = 0; i < count; ) {
            written = mprWriteSocket(sp, start, len);
            if (written < 0) {
                if (total > 0) {
                    break;
                }
                return written;
            } else if (written == 0) {
                break;
            } else {
                len -= written;
                start += written;
                total += written;
                if (len <= 0) {
                    i++;
                    start = iovec[i].start;
                    len = (int) iovec[i].len;
                }
            }
        }
        return total;
    }
}


#if !BIT_ROM
#if !LINUX || __UCLIBC__
static ssize localSendfile(MprSocket *sp, MprFile *file, MprOff offset, ssize len)
{
    char    buf[BIT_MAX_BUFFER];

    mprSeekFile(file, SEEK_SET, (int) offset);
    len = min(len, sizeof(buf));
    if ((len = mprReadFile(file, buf, len)) < 0) {
        assert(0);
        return MPR_ERR_CANT_READ;
    }
    return mprWriteSocket(sp, buf, len);
}
#endif


/*
    Write data from a file to a socket. Includes the ability to write header before and after the file data.
    Works even with a null "file" to just output the headers.
 */
PUBLIC MprOff mprSendFileToSocket(MprSocket *sock, MprFile *file, MprOff offset, MprOff bytes, MprIOVec *beforeVec, 
    int beforeCount, MprIOVec *afterVec, int afterCount)
{
#if MACOSX && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
    struct sf_hdtr  def;
#endif
    MprOff          written, toWriteFile;
    ssize           i, rc, toWriteBefore, toWriteAfter, nbytes;
    int             done;

    rc = 0;

#if MACOSX && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
    def.hdr_cnt = (int) beforeCount;
    def.headers = (beforeCount > 0) ? (struct iovec*) beforeVec: 0;
    def.trl_cnt = (int) afterCount;
    def.trailers = (afterCount > 0) ? (struct iovec*) afterVec: 0;

    if (file && file->fd >= 0) {
        written = bytes;
        if (sock->flags & MPR_SOCKET_BLOCK) {
            mprYield(MPR_YIELD_STICKY);
        }
        rc = sendfile(file->fd, sock->fd, offset, &written, &def, 0);
        if (sock->flags & MPR_SOCKET_BLOCK) {
            mprResetYield();
        }
    } else
#else
    if (1) 
#endif
    {
        /* Either !MACOSX or no file */
        done = 0;
        written = 0;
        for (i = toWriteBefore = 0; i < beforeCount; i++) {
            toWriteBefore += beforeVec[i].len;
        }
        for (i = toWriteAfter = 0; i < afterCount; i++) {
            toWriteAfter += afterVec[i].len;
        }
        toWriteFile = (bytes - toWriteBefore - toWriteAfter);
        assert(toWriteFile >= 0);

        /*
            Linux sendfile does not have the integrated ability to send headers. Must do it separately here.
            I/O requests may return short (write fewer than requested bytes).
         */
        if (beforeCount > 0) {
            rc = mprWriteSocketVector(sock, beforeVec, beforeCount);
            if (rc > 0) {
                written += rc;
            }
            if (rc != toWriteBefore) {
                done++;
            }
        }

        if (!done && toWriteFile > 0 && file->fd >= 0) {
#if LINUX && !__UCLIBC__ && !BIT_HAS_OFF64
            off_t off = (off_t) offset;
#endif
            while (!done && toWriteFile > 0) {
                nbytes = (ssize) min(MAXSSIZE, toWriteFile);
                if (sock->flags & MPR_SOCKET_BLOCK) {
                    mprYield(MPR_YIELD_STICKY);
                }
#if LINUX && !__UCLIBC__
    #if BIT_HAS_OFF64
                rc = sendfile64(sock->fd, file->fd, &offset, nbytes);
    #else
                rc = sendfile(sock->fd, file->fd, &off, nbytes);
    #endif
#else
                rc = localSendfile(sock, file, offset, nbytes);
#endif
                if (sock->flags & MPR_SOCKET_BLOCK) {
                    mprResetYield();
                }
                if (rc > 0) {
                    written += rc;
                    toWriteFile -= rc;
                }
                if (rc != nbytes) {
                    done++;
                    break;
                }
            }
        }
        if (!done && afterCount > 0) {
            rc = mprWriteSocketVector(sock, afterVec, afterCount);
            if (rc > 0) {
                written += rc;
            }
        }
    }
    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return written;
        }
        return -1;
    }
    return written;
}
#endif /* !BIT_ROM */


static ssize flushSocket(MprSocket *sp)
{
    return 0;
}


PUBLIC ssize mprFlushSocket(MprSocket *sp)
{
    if (sp->provider == 0) {
        return MPR_ERR_NOT_INITIALIZED;
    }
    return sp->provider->flushSocket(sp);
}


static char *socketState(MprSocket *sp)
{
    return MPR->emptyString;
}


PUBLIC char *mprGetSocketState(MprSocket *sp)
{
    if (sp->provider == 0) {
        return 0;
    }
    return sp->provider->socketState(sp);
}


PUBLIC bool mprSocketHasBufferedRead(MprSocket *sp)
{
    return (sp->flags & MPR_SOCKET_BUFFERED_READ) ? 1 : 0;
}


PUBLIC bool mprSocketHasBufferedWrite(MprSocket *sp)
{
    return (sp->flags & MPR_SOCKET_BUFFERED_WRITE) ? 1 : 0;
}


PUBLIC bool mprSocketHandshaking(MprSocket *sp)
{
    return (sp->flags & MPR_SOCKET_HANDSHAKING) ? 1 : 0;
}


/*
    Return true if end of file
 */
PUBLIC bool mprIsSocketEof(MprSocket *sp)
{
    return ((sp->flags & MPR_SOCKET_EOF) != 0);
}


/*
    Set the EOF condition
 */
PUBLIC void mprSetSocketEof(MprSocket *sp, bool eof)
{
    if (eof) {
        sp->flags |= MPR_SOCKET_EOF;
    } else {
        sp->flags &= ~MPR_SOCKET_EOF;
    }
}


/*
    Return the O/S socket file handle
 */
PUBLIC Socket mprGetSocketFd(MprSocket *sp)
{
    return sp->fd;
}


/*
    Return the blocking mode of the socket
 */
PUBLIC bool mprGetSocketBlockingMode(MprSocket *sp)
{
    assert(sp);
    return sp && (sp->flags & MPR_SOCKET_BLOCK);
}


/*
    Get the socket flags
 */
PUBLIC int mprGetSocketFlags(MprSocket *sp)
{
    return sp->flags;
}


/*
    Set whether the socket blocks or not on read/write
 */
PUBLIC int mprSetSocketBlockingMode(MprSocket *sp, bool on)
{
    int     oldMode;

    assert(sp);

    lock(sp);
    oldMode = sp->flags & MPR_SOCKET_BLOCK;

    sp->flags &= ~(MPR_SOCKET_BLOCK);
    if (on) {
        sp->flags |= MPR_SOCKET_BLOCK;
    }
#if BIT_WIN_LIKE
{
    int flag = (sp->flags & MPR_SOCKET_BLOCK) ? 0 : 1;
    ioctlsocket(sp->fd, FIONBIO, (ulong*) &flag);
}
#elif VXWORKS
{
    int flag = (sp->flags & MPR_SOCKET_BLOCK) ? 0 : 1;
    ioctl(sp->fd, FIONBIO, (int) &flag);
}
#else
    if (on) {
        fcntl(sp->fd, F_SETFL, fcntl(sp->fd, F_GETFL) & ~O_NONBLOCK);
    } else {
        fcntl(sp->fd, F_SETFL, fcntl(sp->fd, F_GETFL) | O_NONBLOCK);
    }
#endif
    unlock(sp);
    return oldMode;
}


/*
    Set the TCP delay behavior (nagle algorithm)
 */
PUBLIC int mprSetSocketNoDelay(MprSocket *sp, bool on)
{
    int     oldDelay;

    lock(sp);
    oldDelay = sp->flags & MPR_SOCKET_NODELAY;
    if (on) {
        sp->flags |= MPR_SOCKET_NODELAY;
    } else {
        sp->flags &= ~(MPR_SOCKET_NODELAY);
    }
#if BIT_WIN_LIKE
    {
        BOOL    noDelay;
        noDelay = on ? 1 : 0;
        setsockopt(sp->fd, IPPROTO_TCP, TCP_NODELAY, (FAR char*) &noDelay, sizeof(BOOL));
    }
#else
    {
        int     noDelay;
        noDelay = on ? 1 : 0;
        setsockopt(sp->fd, IPPROTO_TCP, TCP_NODELAY, (char*) &noDelay, sizeof(int));
    }
#endif /* BIT_WIN_LIKE */
    unlock(sp);
    return oldDelay;
}


/*
    Get the port number
 */
PUBLIC int mprGetSocketPort(MprSocket *sp)
{
    return sp->port;
}


/*
    Map the O/S error code to portable error codes.
 */
PUBLIC int mprGetSocketError(MprSocket *sp)
{
#if BIT_WIN_LIKE
    int     rc;
    switch (rc = WSAGetLastError()) {
    case WSAEINTR:
        return EINTR;

    case WSAENETDOWN:
        return ENETDOWN;

    case WSAEWOULDBLOCK:
        return EWOULDBLOCK;

    case WSAEPROCLIM:
        return EAGAIN;

    case WSAECONNRESET:
    case WSAECONNABORTED:
        return ECONNRESET;

    case WSAECONNREFUSED:
        return ECONNREFUSED;

    case WSAEADDRINUSE:
        return EADDRINUSE;
    default:
        return EINVAL;
    }
#else
    return errno;
#endif
}


#if BIT_HAS_GETADDRINFO
/*
    Get a socket address from a host/port combination. If a host provides both IPv4 and IPv6 addresses, 
    prefer the IPv4 address.
 */
PUBLIC int mprGetSocketInfo(cchar *ip, int port, int *family, int *protocol, struct sockaddr **addr, socklen_t *addrlen)
{
    MprSocketService    *ss;
    struct addrinfo     hints, *res, *r;
    char                *portStr;
    int                 v6;

    assert(addr);
    ss = MPR->socketService;

    lock(ss);
    memset((char*) &hints, '\0', sizeof(hints));

    /*
        Note that IPv6 does not support broadcast, there is no 255.255.255.255 equivalent.
        Multicast can be used over a specific link, but the user must provide that address plus %scope_id.
     */
    if (ip == 0 || ip[0] == '\0') {
        ip = 0;
        hints.ai_flags |= AI_PASSIVE;           /* Bind to 0.0.0.0 and :: if available */
    }
    v6 = ipv6(ip);
    hints.ai_socktype = SOCK_STREAM;
    if (ip) {
        hints.ai_family = v6 ? AF_INET6 : AF_INET;
    } else {
        hints.ai_family = AF_UNSPEC;
    }
    portStr = itos(port);

    /*
        Try to sleuth the address to avoid duplicate address lookups. Then try IPv4 first then IPv6.
     */
    res = 0;
    if (getaddrinfo(ip, portStr, &hints, &res) != 0) {
        unlock(ss);
        return MPR_ERR_CANT_OPEN;
    }
    /*
        Prefer IPv4 if IPv6 not requested
     */
    for (r = res; r; r = r->ai_next) {
        if (v6) {
            if (r->ai_family == AF_INET6) {
                break;
            }
        } else {
            if (r->ai_family == AF_INET) {
                break;
            }
        }
    }
    if (r == NULL) {
        r = res;
    }
    *addr = mprAlloc(sizeof(struct sockaddr_storage));
    mprMemcpy((char*) *addr, sizeof(struct sockaddr_storage), (char*) r->ai_addr, (int) r->ai_addrlen);

    *addrlen = (int) r->ai_addrlen;
    *family = r->ai_family;
    *protocol = r->ai_protocol;

    freeaddrinfo(res);
    unlock(ss);
    return 0;
}
#else

PUBLIC int mprGetSocketInfo(cchar *ip, int port, int *family, int *protocol, struct sockaddr **addr, Socklen *addrlen)
{
    MprSocketService    *ss;
    struct sockaddr_in  *sa;

    ss = MPR->socketService;

    if ((sa = mprAllocStruct(struct sockaddr_in)) == 0) {
        assert(!MPR_ERR_MEMORY);
        return MPR_ERR_MEMORY;
    }
    memset((char*) sa, '\0', sizeof(struct sockaddr_in));
    sa->sin_family = AF_INET;
    sa->sin_port = htons((short) (port & 0xFFFF));

    if (strcmp(ip, "") != 0) {
        sa->sin_addr.s_addr = inet_addr((char*) ip);
    } else {
        sa->sin_addr.s_addr = INADDR_ANY;
    }

    /*
        gethostbyname is not thread safe on some systems
     */
    lock(ss);
    if (sa->sin_addr.s_addr == INADDR_NONE) {
#if VXWORKS
        /*
            VxWorks only supports one interface and this code only supports IPv4
         */
        sa->sin_addr.s_addr = (ulong) hostGetByName((char*) ip);
        if (sa->sin_addr.s_addr < 0) {
            unlock(ss);
            assert(0);
            return 0;
        }
#else
        struct hostent *hostent;
        hostent = gethostbyname2(ip, AF_INET);
        if (hostent == 0) {
            hostent = gethostbyname2(ip, AF_INET6);
            if (hostent == 0) {
                unlock(ss);
                return MPR_ERR_CANT_FIND;
            }
        }
        memcpy((char*) &sa->sin_addr, (char*) hostent->h_addr_list[0], (ssize) hostent->h_length);
#endif
    }
    *addr = (struct sockaddr*) sa;
    *addrlen = sizeof(struct sockaddr_in);
    *family = sa->sin_family;
    *protocol = 0;
    unlock(ss);
    return 0;
}
#endif


/*
    Return a numerical IP address and port for the given socket info
 */
static int getSocketIpAddr(struct sockaddr *addr, int addrlen, char *ip, int ipLen, int *port)
{
#if (BIT_UNIX_LIKE || WIN)
    char    service[NI_MAXSERV];

#ifdef IN6_IS_ADDR_V4MAPPED
    if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*) addr;
        if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
            struct sockaddr_in addr4;
            memset(&addr4, 0, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = addr6->sin6_port;
            memcpy(&addr4.sin_addr.s_addr, addr6->sin6_addr.s6_addr + 12, sizeof(addr4.sin_addr.s_addr));
            memcpy(addr, &addr4, sizeof(addr4));
            addrlen = sizeof(addr4);
        }
    }
#endif
    if (getnameinfo(addr, addrlen, ip, ipLen, service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV | NI_NOFQDN)) {
        return MPR_ERR_BAD_VALUE;
    }
    *port = atoi(service);

#else
    struct sockaddr_in  *sa;

#if HAVE_NTOA_R
    sa = (struct sockaddr_in*) addr;
    inet_ntoa_r(sa->sin_addr, ip, ipLen);
#else
    uchar   *cp;
    sa = (struct sockaddr_in*) addr;
    cp = (uchar*) &sa->sin_addr;
    fmt(ip, ipLen, "%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);
#endif
    *port = ntohs(sa->sin_port);
#endif
    return 0;
}


/*
    Looks like an IPv6 address if it has 2 or more colons
 */
static int ipv6(cchar *ip)
{
    cchar   *cp;
    int     colons;

    if (ip == 0 || *ip == 0) {
        /*
            Listening on just a bare port means IPv4 only.
         */
        return 0;
    }
    colons = 0;
    for (cp = (char*) ip; ((*cp != '\0') && (colons < 2)) ; cp++) {
        if (*cp == ':') {
            colons++;
        }
    }
    return colons >= 2;
}


/*
    Parse address and return the IP address and port components. Handles ipv4 and ipv6 addresses. 
    If the IP portion is absent, *pip is set to null. If the port portion is absent, port is set to the defaultPort.
    If a ":*" port specifier is used, *pport is set to -1;
    When an address contains an ipv6 port it should be written as:

        aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh:iiii
    or
        [aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh:iiii]:port

    If supplied an IPv6 address, the backets are stripped in the returned IP address.
    This routine parses any "https://" prefix.
 */
PUBLIC int mprParseSocketAddress(cchar *address, char **pip, int *pport, int *psecure, int defaultPort)
{
    char    *ip, *cp;
    int     port;

    ip = 0;
    if (defaultPort < 0) {
        defaultPort = 80;
    }
    if (psecure) {
        *psecure = sncmp(address, "https", 5);
    }
    ip = sclone(address);
    if ((cp = strchr(ip, ' ')) != 0) {
        *cp++ = '\0';
    }
    if ((cp = strstr(ip, "://")) != 0) {
        ip = sclone(&cp[3]);
    }
    if (ipv6(ip)) {
        /*
            IPv6. If port is present, it will follow a closing bracket ']'
         */
        if ((cp = strchr(ip, ']')) != 0) {
            cp++;
            if ((*cp) && (*cp == ':')) {
                port = (*++cp == '*') ? -1 : atoi(cp);

                /* Set ipAddr to ipv6 address without brackets */
                ip = sclone(ip + 1);
                cp = strchr(ip, ']');
                *cp = '\0';

            } else {
                /* Handles [a:b:c:d:e:f:g:h:i] case (no port)- should not occur */
                ip = sclone(ip + 1);
                if ((cp = strchr(ip, ']')) != 0) {
                    *cp = '\0';
                }
                if (*ip == '\0') {
                    ip = 0;
                }
                /* No port present, use callers default */
                port = defaultPort;
            }
        } else {
            /* Handles a:b:c:d:e:f:g:h:i case (no port) */
            /* No port present, use callers default */
            port = defaultPort;
        }

    } else {
        /*
            ipv4 
         */
        if ((cp = strchr(ip, ':')) != 0) {
            *cp++ = '\0';
            if (*cp == '*') {
                port = -1;
            } else {
                port = atoi(cp);
            }
            if (*ip == '*') {
                ip = 0;
            }

        } else if (strchr(ip, '.')) {
            if ((cp = strchr(ip, ' ')) != 0) {
                *cp++ = '\0';
            }
            port = defaultPort;

        } else {
            if (isdigit((uchar) *ip)) {
                port = atoi(ip);
                ip = 0;
            } else {
                /* No port present, use callers default */
                port = defaultPort;
            }
        }
    }
    if (pport) {
        *pport = port;
    }
    if (pip) {
        *pip = ip;
    }
    return 0;
}


PUBLIC bool mprIsSocketSecure(MprSocket *sp)
{
    return sp->sslSocket != 0;
}


PUBLIC bool mprIsSocketV6(MprSocket *sp)
{
    return sp->ip && ipv6(sp->ip);
}


PUBLIC bool mprIsIPv6(cchar *ip)
{
    return ip && ipv6(ip);
}


PUBLIC void mprSetSocketPrebindCallback(MprSocketPrebind callback)
{
    MPR->socketService->prebind = callback;
}


static void manageSsl(MprSsl *ssl, int flags) 
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(ssl->key);
        mprMark(ssl->keyFile);
        mprMark(ssl->certFile);
        mprMark(ssl->caFile);
        mprMark(ssl->caPath);
        mprMark(ssl->ciphers);
        mprMark(ssl->mutex);
        mprMark(ssl->config);
        mprMark(ssl->provider);
        mprMark(ssl->providerName);
    }
}


/*
    Create a new SSL context object
 */
PUBLIC MprSsl *mprCreateSsl(int server)
{
    MprSsl      *ssl;

    if ((ssl = mprAllocObj(MprSsl, manageSsl)) == 0) {
        return 0;
    }
    ssl->protocols = MPR_PROTO_TLSV1 | MPR_PROTO_TLSV11 | MPR_PROTO_TLSV12;

    /*
        The default for servers is not to verify client certificates.
        The default for clients is to verify unless MPR->verifySsl has been set to false
     */
    if (server) {
        ssl->verifyDepth = 10;
        ssl->verifyPeer = 0;
        ssl->verifyIssuer = 0;
    } else {
        ssl->verifyDepth = 10;
        ssl->verifyPeer = MPR->verifySsl;
        ssl->verifyIssuer = MPR->verifySsl;
        ssl->caFile = mprJoinPath(mprGetAppDir(), MPR_CA_CERT);
    }
    ssl->mutex = mprCreateLock();
    return ssl;
}


/*
    Clone a SSL context object
 */
PUBLIC MprSsl *mprCloneSsl(MprSsl *src)
{
    MprSsl      *ssl;

    if ((ssl = mprAllocObj(MprSsl, manageSsl)) == 0) {
        return 0;
    }
    if (src) {
        *ssl = *src;
    }
    return ssl;
}


PUBLIC int mprLoadSsl()
{
#if BIT_PACK_SSL
    MprSocketService    *ss;
    MprModule           *mp;
    cchar               *path;

    ss = MPR->socketService;
    if (ss->providers) {
        return 0;
    }
    path = mprJoinPath(mprGetAppDir(), "libmprssl");
    if ((mp = mprCreateModule("sslModule", path, "mprSslInit", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    if (mprLoadModule(mp) < 0) {
        mprError("Cannot load %s", path);
        return MPR_ERR_CANT_READ;
    }
    return 0;
#else
    mprError("SSL communications support not included in build");
    return MPR_ERR_BAD_STATE;
#endif
}


static int loadProviders()
{
    MprSocketService    *ss;

    ss = MPR->socketService;
    mprGlobalLock();
    if (!ss->providers && mprLoadSsl() < 0) {
        mprGlobalUnlock();
        return MPR_ERR_CANT_READ;
    }
    if (!ss->providers) {
        mprError("Cannot load SSL provider");
        mprGlobalUnlock();
        return MPR_ERR_CANT_INITIALIZE;
    }
    mprGlobalUnlock();
    return 0;
}


/*
    Upgrade a socket to use SSL
 */
PUBLIC int mprUpgradeSocket(MprSocket *sp, MprSsl *ssl, cchar *peerName)
{
    MprSocketService    *ss;
    char                *providerName;

    ss  = sp->service;
    assert(sp);

    if (!ssl) {
        return MPR_ERR_BAD_ARGS;
    }
    if (!ssl->provider) {
        if (loadProviders() < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
        providerName = (ssl->providerName) ? ssl->providerName : ss->sslProvider;
        if ((ssl->provider = mprLookupKey(ss->providers, providerName)) == 0) {
            mprError("Cannot use SSL, missing SSL provider %s", providerName);
            return MPR_ERR_CANT_INITIALIZE;
        }
        ssl->providerName = providerName;
    }
    mprLog(4, "Using SSL provider: %s", ssl->providerName);
    sp->provider = ssl->provider;
#if FUTURE
    /* session resumption can cause problems with Nagle. However, appweb opens sockets with nodelay by default */
    sp->flags |= MPR_SOCKET_NODELAY;
    mprSetSocketNoDelay(sp, 1);
#endif
    return sp->provider->upgradeSocket(sp, ssl, peerName);
}


PUBLIC void mprAddSslCiphers(MprSsl *ssl, cchar *ciphers)
{
    assert(ssl);
    if (ssl->ciphers) {
        ssl->ciphers = sjoin(ssl->ciphers, ":", ciphers, NULL);
    } else {
        ssl->ciphers = sclone(ciphers);
    }
    ssl->changed = 1;
}


PUBLIC void mprSetSslCiphers(MprSsl *ssl, cchar *ciphers)
{
    assert(ssl);
    ssl->ciphers = sclone(ciphers);
    ssl->changed = 1;
}


PUBLIC void mprSetSslKeyFile(MprSsl *ssl, cchar *keyFile)
{
    assert(ssl);
    ssl->keyFile = (keyFile && *keyFile) ? sclone(keyFile) : 0;
    ssl->changed = 1;
}


PUBLIC void mprSetSslCertFile(MprSsl *ssl, cchar *certFile)
{
    assert(ssl);
    ssl->certFile = (certFile && *certFile) ? sclone(certFile) : 0;
    ssl->changed = 1;
}


PUBLIC void mprSetSslCaFile(MprSsl *ssl, cchar *caFile)
{
    assert(ssl);
    ssl->caFile = (caFile && *caFile) ? sclone(caFile) : 0;
    ssl->changed = 1;
}


/* Only supported in OpenSSL */
PUBLIC void mprSetSslCaPath(MprSsl *ssl, cchar *caPath)
{
    assert(ssl);
    ssl->caPath = (caPath && *caPath) ? sclone(caPath) : 0;
    ssl->changed = 1;
}


/* Only supported in OpenSSL */
PUBLIC void mprSetSslProtocols(MprSsl *ssl, int protocols)
{
    assert(ssl);
    ssl->protocols = protocols;
    ssl->changed = 1;
}


PUBLIC void mprSetSslProvider(MprSsl *ssl, cchar *provider)
{
    assert(ssl);
    ssl->providerName = (provider && *provider) ? sclone(provider) : 0;
    ssl->changed = 1;
}


PUBLIC void mprVerifySslPeer(MprSsl *ssl, bool on)
{
    if (ssl) {
        ssl->verifyPeer = on;
        ssl->verifyIssuer = on;
        ssl->changed = 1;
    } else {
        MPR->verifySsl = on;
    }
}


PUBLIC void mprVerifySslIssuer(MprSsl *ssl, bool on)
{
    assert(ssl);
    ssl->verifyIssuer = on;
    ssl->changed = 1;
}


PUBLIC void mprVerifySslDepth(MprSsl *ssl, int depth)
{
    assert(ssl);
    ssl->verifyDepth = depth;
    ssl->changed = 1;
}


/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
