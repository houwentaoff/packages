/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, xxx.
 *       Filename:  
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), 544088192@qq.com
 *   Organization:  xxx
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "if_alg.h"

#include "bsreader.h"

#define NO_THREAD

#define DATA_ALIGN

#define KEY_LEN    16
#define IV_LEN    16
#define SOCK_CMD_HEAD_LEN   4

/* #####   DEBUG MACROS   ########################################################### */
#define     INOUT_DEBUG                          0  /*1 is open , o is close */
#define     CRYPTO_DEBUG                          1  /*1 is open , 0 is close */
#define     CRYPTO_ERROR                          1  /*1 is open , 0 is close */

#define MODULE_NAME "xxx"

#if INOUT_DEBUG
#define FUN_IN(fmt, args...)              printf("["MODULE_NAME"]---> %s %s():"fmt"", __FILE__, __func__, ##args)/*  */
#define FUN_OUT(fmt, args...)             printf("["MODULE_NAME"]<--- %s %s():"fmt"", __FILE__, __func__, ##args) /*  */
#else
#define FUN_IN(fmt, args...)
#define FUN_OUT(fmt, args...)
#endif

#if CRYPTO_DEBUG
#define PRT_DBG(fmt, args...)             printf("["MODULE_NAME"]--->%s():line[%d]"fmt"", __func__, __LINE__, ##args)/*  */
#else
#define PRT_DBG(fmt, args...)
#endif

#if CRYPTO_ERROR
#define PRT_ERR(fmt, args...)                                                               \
    do                                                                                          \
{                                                                                           \
    printf("\033[5;41;32m [ERROR] %s ---> %s ():line[%d]:\033[0m", __FILE__, __func__, __LINE__);      \
    printf(" "fmt, ##args);                                                                 \
}while(0)    /* */
#else
#define PRT_ERR(fmt, args...)
#endif



int do_crypto(void *);

static void (*func)(void *) proc[3] = {0};

uint32_t Crypto(void * params)
{
    uint32_t uiRet = 0;
    FUN_IN();
    if (do_crypto(params) < 0)
    {
        uiRet = 2;
    }
    FUN_OUT();
    return uiRet;
}

int crypto_impl(struct sockaddr_alg *sa, unsigned char *key, unsigned char *usr_iv, 
        bsreader_frame_info_t *frame_info, int encrypt)
{
    int tfmfd = 0;
    int cryptofd = 0;
    struct msghdr msg = {};
    struct cmsghdr *cmsg;
    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(20)] = {};
    struct af_alg_iv *iv;
    struct iovec iov;
    int ret = 0;
    u32 len = 0;
    
    PRT_DBG("encrypt[%d]\n", encrypt);

    tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (tfmfd < 0)
    {
        perror("socket init  fail");
        ret = -1;
        goto err;
    }
    bind(tfmfd, (struct sockaddr *)sa, sizeof(struct sockaddr_alg));

    setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY, key, KEY_LEN);
    cryptofd = accept(tfmfd, NULL, 0);
    if (cryptofd < 0)
    {
        perror("accept fail");
        ret = -2;
        goto err;
    }
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type = ALG_SET_OP;
    cmsg->cmsg_len = CMSG_LEN(SOCK_CMD_HEAD_LEN);

    if (encrypt)
        *(__u32 *)CMSG_DATA(cmsg) = ALG_OP_ENCRYPT; 
    else
        *(__u32 *)CMSG_DATA(cmsg) = ALG_OP_DECRYPT; 

    cmsg = CMSG_NXTHDR(&msg, cmsg); 
    cmsg->cmsg_level = SOL_ALG; 
    cmsg->cmsg_type = ALG_SET_IV; 

    cmsg->cmsg_len = CMSG_LEN(SOCK_CMD_HEAD_LEN + IV_LEN); 
    iv = (void *)CMSG_DATA(cmsg); 
    memcpy(iv->iv, usr_iv, IV_LEN);
    iv->ivlen = IV_LEN;
    iov.iov_base = frame_info->src;
    len = frame_info->frame_size;
    if (len < 16)
    {
#ifdef DATA_ALIGN
        len = 16;
#else
        len = len;
#endif
    }
    iov.iov_len = len;//frame_info->frame_size;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = sendmsg(cryptofd, &msg, 0);
    if (ret < 0)
    {
        perror("send err");
        ret = -2;
        goto err;
    }
    frame_info->bs_info.mfd = tfmfd;
    frame_info->bs_info.sfd = cryptofd;
    PRT_DBG("encrypt[%d]\n", encrypt);
    return ret;
err:
    if (cryptofd)close(cryptofd);
    if (tfmfd)close(tfmfd);
    return ret;
}
static int inline fill_one_frame(bsreader_frame_info_t * frame_info, xxx *params)
{
    int ret = 0;
    frame_info->src = params->pucData;
    frame_info->bs_info.dir = params->ulDir;
    frame_info->bs_info.cb = params->ulDir ? proc[0]:proc[1];
    frame_info->bs_info.crypto_type = params->ulType;
    frame_info->bs_info.data = params->pstMessage;

    frame_info->frame_addr = params->pucDataOut;
    frame_info->frame_size = params->ulDataBitLength/8;
    return ret;
}
#ifdef NO_THREAD
#define  QUEUE_SIZE    10
/*
 * 队列演示（队列的几个函数）
 * */
typedef struct {
    bsreader_frame_info_t arr[QUEUE_SIZE];
    int head; //记录最前面数字所在的下标
    int tail; //记录最后一个有效数字的下一个坐标
} Queue;

void queue_init(Queue *p_queue) {
    p_queue->head = 0;
    p_queue->tail = 0;
}

void queue_deinit(Queue *p_queue) {
    p_queue->head = 0;
    p_queue->head = 0;
}

int queue_size(const Queue *p_queue) {
    return (p_queue->tail - p_queue->head);
}

int queue_empty(const Queue *p_queue) {
    return !(p_queue->tail - p_queue->head);
}

int queue_full(const Queue *p_queue) {
    return (p_queue->tail + 1) % (QUEUE_SIZE) == p_queue->head;
}

int queue_push(Queue *p_queue, bsreader_frame_info_t val) {
    if (queue_full(p_queue)) {
        return 0;
    }
    else {
        p_queue->arr[p_queue->tail] = val;
        p_queue->tail = (p_queue->tail+1) % (QUEUE_SIZE);
        return 1;
    }
}

int queue_pop(Queue *p_queue, bsreader_frame_info_t *p_num) {
    if (queue_empty(p_queue)) {
        return 0;
    }
    else {
        *p_num = p_queue->arr[p_queue->head];
        p_queue->head =  (p_queue->head + 1)%(QUEUE_SIZE);
        return 1;
    }
}

int queue_front(const Queue *p_queue, bsreader_frame_info_t *p_num) {
    if (queue_empty(p_queue)) {
        return 0;
    }
    else {
        *p_num = p_queue->arr[p_queue->head];
        return 1;
    }
}

static Queue qu;
#if 0
void main() {
    int y;
    queue_init(&qu);
    queue_push(&qu, 1);
    queue_push(&qu, 2);
    queue_push(&qu, 3);
    queue_push(&qu, 4);



    printf(">>>%d\r\n", queue_size(&qu));
    // 拿出一个数据，数据还在
    queue_front(&qu, &y);
    printf(">%d\r\n", y);
    printf(">>%d\r\n", queue_size(&qu));
    // 取出一个数据，数据已经被删除
    queue_pop(&qu, &y);
    printf(">%d\r\n", y);
    printf(">>%d\r\n", queue_size(&qu));
    // 插入一个数据
    queue_push(&qu, 5);

    queue_pop(&qu, &y);
    printf(">%d\r\n", y);
    printf(">>%d\r\n", queue_size(&qu));

    //	while (qu.tail != qu.head) {
    //		qu.head = qu.head + 1;
    //		printf("当前队列值=%d\n", qu.arr[qu.head]);
    //	}
}
#endif

int crypto_req ()
{
    fd_set read_fds;  //读文件操作符
    fd_set exception_fds; //异常文件操作符
    bsreader_frame_info_t frame_info;
    
    int ret = 0;
    int len = 0;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1;
    do {
        if (0 == queue_front(&qu, &frame_info))
        {
			PRT_ERR("queue is empty\n");
            return 0;
        }
        FD_ZERO(&read_fds);
        FD_ZERO(&exception_fds);
        FD_SET(frame_info.bs_info.sfd,&read_fds);
        FD_SET(frame_info.bs_info.sfd,&exception_fds);
        ret = select(frame_info.bs_info.sfd+1,&read_fds,NULL,&exception_fds, &tv);
        if(ret < 0)
        {
			queue_pop(&qu, &frame_info);
            PRT_ERR("Fail to select!\n");
            return -1;
        }
		else if (ret == 0)
		{
			PRT_DBG("%d : %d\n", qu.head, qu.tail);
			return 0;
		}
        if(FD_ISSET(frame_info.bs_info.sfd, &read_fds))
        {
            len = frame_info.frame_size;
            if (len < 16)
            {
#ifdef DATA_ALIGN
                len = 16;
#else 
                len = len;
#endif
            }
            ret = read(frame_info.bs_info.sfd, frame_info.frame_addr, len);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes of normal data:\n", ret);

            queue_pop(&qu, &frame_info);
            if (frame_info.bs_info.sfd)
                close(frame_info.bs_info.sfd);
            if (frame_info.bs_info.mfd)
                close(frame_info.bs_info.mfd);
            frame_info.bs_info.cb((MESSAGE_S *)frame_info.bs_info.data);
            if (frame_info.src)
            {
                free(frame_info.src);
            }
            continue;
        }
        else if(FD_ISSET(frame_info.bs_info.sfd,&exception_fds)) //异常事件
        {
            PRT_ERR("get %d bytes of exception\n",ret);
            continue;
        }
        else
        {
            break;
        }
    }while (!queue_empty(&qu));
	PRT_DBG("queue is empty\n");
    return 0;
}
#endif
//crypt and enqueue
int do_crypto(void *params)
{
    bsreader_frame_info_t frame_info;

    struct sockaddr_alg sa = {
        .salg_family = AF_ALG,
        .salg_type = "skcipher",
        .salg_name = "aes-xxx"
    };
    int ret = 0;
    int encrypt = 0;
    unsigned char *key = NULL;
    unsigned char usr_iv[IV_LEN]={0};//ONLY 6 AVALIB
    unsigned char * p = NULL;
    
    if (!params)
    {
        crypto_req();
        return 0;
    }
    memset(&frame_info, 0, sizeof(bsreader_frame_info_t));


    switch(params->ulType)
    {
        case 1:
            strcpy((char *)sa.salg_name, "111");
            //gen IV

            break;
        case 2:
            //gen IV
            *(uint32_t *)&usr_iv[0] = params->ulCount;
            strcpy((char *)sa.salg_name, "222");
            break;
        case 3:
            strcpy((char *)sa.salg_name, "ctr(aes)");
            break;
        default:
            PRT_ERR("unknow crypto type\n");
            break;
        return -1;
    }
    fill_one_frame(&frame_info, params);

    crypto_impl(&sa, key, usr_iv, &frame_info, encrypt);

#ifdef NO_THREAD
    //将frame_info 入简单队列
#if 1
    //frame_info.src
    if (frame_info.frame_size < 16)
    {
#ifdef DATA_ALIGN
        p = malloc(16);//frame_info.frame_size);
        memset((void *)p, 0, 16);
#else 
        p = malloc(frame_info.frame_size);
#endif
    }
    else
    {
        p = malloc(frame_info.frame_size);
    }
    if (!p)
    {
        perror("malloc err!!!!!!");
        PRT_ERR("malloc err\n");
    }
    else 
    {
        memcpy(p, frame_info.src, frame_info.frame_size);
        frame_info.src = (u8 *)p;
    }
    queue_push(&qu, frame_info);
    crypto_req();
#else
    if (bsreader_get_one_frame(0, &frame_info) < 0) {
        printf("bs reader gets frame error\n");
        //continue;
        //return -1;
    }
    ret = read(frame_info.bs_info.sfd, frame_info.frame_addr, frame_info.frame_size);
    if (ret < 0)
    {
        perror("read err");
        ret = -2;
        //goto err;
    }
    if (frame_info.bs_info.sfd)
        close(frame_info.bs_info.sfd);
    if (frame_info.bs_info.mfd)
        close(frame_info.bs_info.mfd);
    //(*g_stSpaccParam.proc[pstRespVector->io.encrypt])((MESSAGE_S *)*(((uint32_t *)pstRespVector->io.dst) + 4));
    frame_info.bs_info.cb((MESSAGE_S *)frame_info.bs_info.data);
#endif

#else // bsreader
    // 向ring fifo 写入
    dispatch_and_copy_one_frame(&frame_info);
#endif
    return ret;
}
uint32_t init(uint32_t size, void (*cb1)(void *), void (*cb1)(void *))
{
    FUN_IN();
#ifdef NO_THREAD
    proc[0] = pfnUlpMsgProc;
    proc[1] = pfnDlpMsgProc;
    queue_init(&qu);
#else
    bsreader_init_data_t  init_data;
    proc[0] = cb1;
    proc[1] = cb2;

    memset(&init_data, 0, sizeof(init_data));
    init_data.max_buffered_frame_num = 8;

    if (bsreader_init(&init_data) < 0) {
        printf("bsreader init failed \n");
        return -1;
    }

    if (bsreader_open() < 0) {
        printf("bsreader open failed \n");
        return -1;
    }
    bsreader_reset(0);
#endif
    FUN_OUT();
    return 0;
}
void mod_exit(void)
{
    FUN_IN();
#ifdef NO_THREAD
    queue_deinit(&qu);
#else
    if (bsreader_close() < 0) {
        printf("bsreader_close() failed!\n");
    }
#endif
    FUN_OUT();
}

