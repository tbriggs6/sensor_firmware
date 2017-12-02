#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "bcast.h"


// sometimes, errno exports, sometimes it doesn't
#ifndef OK
#define OK 0
#endif


static int count_matching_sockets(struct addrinfo *servinfo)
{
    struct addrinfo *p;

    int count = 0;
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        count++;
    }

    return count;
}

static int set_max_hops(int sockfd, int max_hops)
{
    int rc;
    rc = setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
               (char*) &max_hops, sizeof(max_hops));
    if (rc != OK) {
        fprintf(stderr,"%s:%d - error could not set max hops: %s\n", __FILE__, __LINE__, strerror(errno));
        return errno;
    }

    return OK;
}

static int get_mcast_addrinfo(const char *address, const char *port, struct addrinfo **mcastAddr)
{
    struct addrinfo hints;
    int rc;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST;

    rc = getaddrinfo(address, port, &hints, mcastAddr);
    if (rc != OK) {
        fprintf(stderr,"%s:%d - error could not get multicast address info: %s\n", __FILE__, __LINE__, strerror(errno));
        return errno;
    }

    return OK;
}

static int set_mcast_interface(int sockfd, int iface)
{
    int rc;
    rc = setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
               (char*) &iface, sizeof(iface));

    if (rc != OK) {
        fprintf(stderr,"%s:%d - error could not set interface: %s\n", __FILE__, __LINE__, strerror(errno));
        return errno;
    }

    return OK;
}

static int create_send_sockets(const char *address, const char *port, struct addrinfo **mcastAddr, int **sockets, int *num_sockets)
{
    int sockfd;
    struct addrinfo hints;

    struct addrinfo *servinfo, *p;
    int rc, i;


    rc = get_mcast_addrinfo(address, port, mcastAddr);
    if (rc != OK) return rc;

      // create an array of sockets
    int max_sockets = count_matching_sockets(*mcastAddr);
    *sockets = (int *) malloc(sizeof(int) * max_sockets);
    *num_sockets = 0;

    //TODO - should this be all matching services?
    // find the first matching service
    for(p = *mcastAddr; p != NULL; p = p->ai_next) {

        int sockfd;
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < OK)
            continue;

        // set the maximum number of hops
        rc = set_max_hops(sockfd, MAX_HOPS);
        if (rc != OK) {
            close(sockfd);
            continue;
        }

        //rc = set_mcast_interface(sockfd, 0);
        //if (rc != OK) {
        //    close(sockfd);
        //    continue;
        //}

        *sockets[*num_sockets] = sockfd;
        *num_sockets = *num_sockets + 1;
    }


     return OK;
}


int bcast_message(int num_sockets, int *sockets, const struct addrinfo const * mcast_address, const char *msg, int len)
{
    int i = 0;
    int success = 0;
    int fail = 0;
    int rc = 0;

    for (i = 0; i < num_sockets; i++) {

        rc = sendto(sockets[i], msg, len, 0, mcast_address->ai_addr, mcast_address->ai_addrlen);
        if (rc < OK) {
            fprintf(stderr,"%s:%d Error sending to socket %d : %s\n", __FILE__, __LINE__, i, strerror(errno));
            fail++;
        }
        success++;
    }

    return fail;
}


typedef struct {
	uint32_t header;
	char message[32];
} echo_t;

int main(int argc, char **argv)
{

    int rc, num_sockets;
    int *sockets;
    int len = 0;
    struct addrinfo *mcast_address;

    if (argc != 3) {
        fprintf(stderr,"Error - specify the message on the command line");
        exit(0);
    }

    len = strlen(argv[2]);

    rc = create_send_sockets(argv[1], "5353", &mcast_address, &sockets, &num_sockets);
    if (rc != OK) {
        fprintf(stderr,"Error - could not create multicast sending sockets.\n");
        exit(-1);
    }

    if (mcast_address == NULL) {
        fprintf(stderr,"Error - mcast address was NULL!\n");
        exit(-1);
    }

    if (num_sockets == 0) {
        fprintf(stderr,"Error - no matching network interfaces found\n");
        exit(-2);
    }

    echo_t echo;
    echo.header =0x323232;
    strncpy((char *) &echo.message, argv[2], 32);

    rc = sendto(sockets[0], &echo, sizeof(echo_t), 0, mcast_address->ai_addr, mcast_address->ai_addrlen);


    // cleanup
    for (int i = 0; i < num_sockets; i++) {
        close(sockets[i]);
    }

    free(sockets);

    freeaddrinfo(mcast_address);
}

