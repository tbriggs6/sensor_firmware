/*
 * recv-bcast.c
 *
 *  Created on: Nov 24, 2017
 *      Author: contiki
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "bcast.h"

// sometimes, errno exports, sometimes it doesn't
#ifndef OK
#define OK 0
#endif

/*! \brief set the receive buffer size for the socket
 *
 * Sets the receive buffer size for the socket to the maximum
 * of the default buffer size and the given receive size
 * @param sock - the socket to modify
 * @param receive_size - the desired buffer size
 */
static int setsock_size(const int sock, const int receive_size)
{
    int status = 0;     // return status
    int default_size = 0;     // optimal buffer size
    int request_size = 0;    // requested buffer size
    int actual_size = 0;    // actual buffer size

    // integer pointers used by the get/set sockopt
    socklen_t default_len = sizeof(default_size);
    socklen_t request_len = sizeof(request_size);
    socklen_t actual_len = sizeof(actual_size);

    // get the default socket size
    status = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &default_size, &default_len);
    if ((status < 0) || (default_len != sizeof(socklen_t))) {
        fprintf(stderr,"%s:%d Error - could not get sock bufsize: %s\n", __FILE__, __LINE__, strerror(errno));
        return errno;
    }

    // take the max of the default and requested sizes
    request_size = (default_size > receive_size) ? default_size : receive_size;

    // nothing further to do, so just return
    if (request_size == default_size)
        return OK;

    // set the size on the socket
    status = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &request_size, request_len);
    if (status < OK) {
        fprintf(stderr,"%s:%d Error - could not get sock bufsize: %s\n", __FILE__, __LINE__,strerror(errno));
        return errno;
    }

    // read the size one more time, see if we got the requested size
    status = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &actual_size, &actual_len);
    if ((status < OK) || (actual_len != sizeof(socklen_t))) {
       fprintf(stderr,"%s:%d Error - could not get sock bufsize: %s\n", __FILE__, __LINE__,strerror(errno));
       return errno;
    }

    if (actual_size != request_size) {
        fprintf(stderr,"%s:%d Warning - requested %d bytes, but actually got %d\n",__FILE__, __LINE__,request_size, actual_size);
    }


    return OK;
}


/*! \brief set the socket reusability
 *
 * A socket that is reusable can receive traffic from more than one client, which
 * is useful for responding to multicast / broadcast traffic.
 *
 * @param sockfd - the socket to modify
 * @param reusable - the reusability (1 = true, 0 = false)
 */
static int setsocket_reusable(const int sockfd, int reusable)
{
    // set socket reusability
    int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reusable, sizeof(int));
    if (status < OK) {
        fprintf(stderr,"%s:%d Error - could not set reusability : %s\n", __FILE__, __LINE__,strerror(errno));
        return errno;
    }

    return OK;
}

/*! \brief create the socket
 *
 * Creates a socket bound to the given IP and port and IPv6 UDP traffic
 *
 * @param mcastiP - the IP address for the multicast address
 * @param port - the multicast port number
 * @param mcastAddr - pointer to the constructed multicast address (will be filled in by this function)
 *
 */
static int create_socket(const char *mcastIP, const char *port, struct addrinfo **mcastAddr)
{
    struct addrinfo hints;
    struct addrinfo *localAddr = NULL;

    int sock;
    int status;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET6;
    hints.ai_flags = AI_NUMERICHOST;


    status = getaddrinfo(mcastIP, NULL, &hints, mcastAddr);
    if (status != OK) {
        fprintf(stderr,"%s:%d Error - getaddrinfo: %s\n", __FILE__, __LINE__, gai_strerror(status));
        goto error;
    }

    hints.ai_family = ((struct addrinfo *) *mcastAddr)->ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, port, &hints, &localAddr);
    if (status != OK) {
        fprintf(stderr,"%s:%d Error - getaddrinfo: %s\n", __FILE__, __LINE__, gai_strerror(status));
        goto error;
    }

    sock = socket(localAddr->ai_family, localAddr->ai_socktype, 0);
    if (sock < OK) {
        fprintf(stderr,"%s:%d Error - could not create socket: %s\n", __FILE__, __LINE__,strerror(errno));
        goto error;
    }

    setsocket_reusable(sock, 1);


    // bind the address to the port
    status = bind(sock, localAddr->ai_addr, localAddr->ai_addrlen);
    if (status != OK) {
        fprintf(stderr,"%s:%d Error - could not set sock options: %s\n", __FILE__, __LINE__,strerror(errno));
        goto error;
    }

    freeaddrinfo(localAddr);
    return sock;

error:
    if (localAddr != NULL) {
        freeaddrinfo(localAddr);
        localAddr = NULL;
    }

    if (mcastAddr != NULL) {
        freeaddrinfo(*mcastAddr);
        mcastAddr = NULL;
    }

    return status;
}


/*! \brief join the IPv6 multi-cast listening group
 *
 * IPv6 implementations have a different strategy for receiving multi-cast
 * messages.  The strategy requres setting a socket option to add
 * the socket to the multi-cast request group.
 *
 * @param sockfd - the socket
 */

static int join_multicast_group(const int sockfd, const struct addrinfo const * mcastAddr)
{
    struct ipv6_mreq mrequest;
    int status;

    memcpy(&mrequest.ipv6mr_multiaddr,
           &((struct sockaddr_in6 *) (mcastAddr->ai_addr))->sin6_addr,
           sizeof(mrequest.ipv6mr_multiaddr));

    mrequest.ipv6mr_interface = 0;

    status = setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                        (char *) &mrequest, sizeof(struct ipv6_mreq));
    if (status != OK) {
        fprintf(stderr,"%s:%d Error - could not set sock options: %s\n", __FILE__, __LINE__,strerror(errno));
        return errno;
    }

    return OK;
}


int create_recv_socket(const char *mcastIP, const char *port, const socklen_t rcvSize)
{
    struct addrinfo *mcastAddr = NULL;
    int status;

    int sockfd = create_socket(mcastIP, port, &mcastAddr);
    if (sockfd < 0) {
        return errno;
    }

    status = setsock_size(sockfd, rcvSize);
    if (status < OK) goto error;

    status = join_multicast_group(sockfd, mcastAddr);
    if (status < OK) goto error;

    if (mcastAddr != NULL)
        freeaddrinfo(mcastAddr);
    return sockfd;


// nothing to deallocate
error:
    if (mcastAddr != NULL)
        freeaddrinfo(mcastAddr);
    return status;
}

int receive_message(int sockfd, char *dest, int maxBytes)
{
    /* Receive a single datagram from the server */
    int bytes = recvfrom(sockfd, dest, maxBytes, 0, NULL, 0);
    if (bytes < OK) {
        fprintf(stderr,"%s:%d Error - could not read from broadcast: %s\n", __FILE__, __LINE__,strerror(errno));
        return errno;
    }

    return bytes;
}

int main(int argc, char **argv)
{


    int sockfd = create_recv_socket(MCAST_ADDR, MCAST_PORT,256);

    while(1) {
        char buff[1024];
        memset(buff, 0, sizeof(buff));

        int bytes = receive_message(sockfd, buff, sizeof(buff));

        printf("%s\n", buff);
    }
}
