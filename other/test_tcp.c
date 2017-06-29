/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Jz.
 *       Filename:  test_tcp.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  06/13/2017 10:54:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Jz
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_SERVER      192.168.27.103   /*  hostname also supported */
#define DEFAULT_PORT_UP     7000
#define DEFAULT_PORT_DW     1782
//#define USETCP

#define STRINGIFY(x)    #x
#define STR(x)          STRINGIFY(x)
#define MSG printf            /*  */

static char serv_addr[64] = STR(DEFAULT_SERVER); /*  address of the server (host name or IPv4/IPv6) */
static char serv_port_up[8] = STR(DEFAULT_PORT_UP); /*  server port for upstream traffic */
static char serv_port_down[8] = STR(DEFAULT_PORT_DW); /*  server port for downstream traffic */
/* network sockets */
static int sock_up; /* socket for upstream traffic */
static int sock_down; /* socket for downstream traffic */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    struct addrinfo hints;
    struct addrinfo *result; /*  store result of getaddrinfo */
    struct addrinfo *q; /*  pointer to move into *result data */
    int i; /*  loop variable and temporary variable for return value */ 
    char buff_up[]={"123456789\n"};
    char host_name[64];
    char port_name[64];
    int buff_index = 10;

    /*  prepare hints to open network sockets */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; /*  WA: Forcing IPv4 as AF_UNSPEC makes connection on localhost to fail */
#ifdef USETCP
    hints.ai_socktype = SOCK_STREAM;
#else
    hints.ai_socktype = SOCK_DGRAM;
#endif
    /*  look for server address w/ upstream port */
    i = getaddrinfo(serv_addr, serv_port_up, &hints, &result);
    if (i != 0) {
        printf("ERROR: [up] getaddrinfo on address %s (PORT %s) returned %s\n", serv_addr, serv_port_up, gai_strerror(i));
        exit(EXIT_FAILURE);
    }
    /* try to open socket for upstream traffic */
    for (q=result; q!=NULL; q=q->ai_next) {
#if 0
        sock_up = socket(AF_INET,SOCK_STREAM,0);
#else
        sock_up = socket(q->ai_family, q->ai_socktype, q->ai_protocol);
#endif
        if (sock_up == -1) continue; /* try next field */
        else break; /* success, get out of loop */
    }
    if (q == NULL) {
        MSG("ERROR: [up] failed to open socket to any of server %s addresses (port %s)\n", serv_addr, serv_port_up);
        i = 1;
        for (q=result; q!=NULL; q=q->ai_next) {
            getnameinfo(q->ai_addr, q->ai_addrlen, host_name, sizeof host_name, port_name, sizeof port_name, NI_NUMERICHOST);
            MSG("INFO: [up] result %i host:%s service:%s\n", i, host_name, port_name);
            ++i;
        }
        exit(EXIT_FAILURE);
    }
#if 0
    struct sockaddr_in addr_ser;  
    bzero(&addr_ser,sizeof(addr_ser));  
    addr_ser.sin_family=AF_INET;  
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);  
    addr_ser.sin_port=htons(DEFAULT_PORT_UP);  
#endif
    /* connect so we can send/receive packet with the server only */
    //    i = connect(sock_up,(struct sockaddr *)&addr_ser,sizeof(addr_ser));  
    //    printf("aa i[%d]\n", i);
    i = connect(sock_up, q->ai_addr, q->ai_addrlen);
    if (i != 0) {
        MSG("ERROR: [up] connect returned %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    unsigned int count = 0;
    while (1)
    {
        if (i != 0) {
            MSG("ERROR: [up] connect returned %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        send(sock_up, (void *)buff_up, buff_index, 0);
        printf("send data count[%d]len[%d]\n", count++, buff_index);
        sleep(1);
    }
    freeaddrinfo(result);
    return EXIT_SUCCESS;
}
