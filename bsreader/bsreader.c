/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, xxx.
 *       Filename:  bsreader.c
 *
 *    Description:  bit stream --> fifo
 *         Others:
 *
 *        Version:  1.0
 *        Created:  09/25/2019 01:05:50 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), 544088192@qq.com
 *   Organization:  xxx
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "fifo.h"
#include "bsreader.h"

/* #####   DEBUG MACROS   ########################################################### */
#define     INOUT_DEBUG                          1  /*1 is open , o is close */
#define     CRYPTO_DEBUG                          1  /*1 is open , 0 is close */
#define     CRYPTO_ERROR                          1  /*1 is open , 0 is close */

#if INOUT_DEBUG
#define FUN_IN(fmt, args...)              printf("---> %s %s():"fmt"\n", __FILE__, __func__, ##args)/*  */
#define FUN_OUT(fmt, args...)             printf("<--- %s %s():"fmt"\n", __FILE__, __func__, ##args) /*  */
#else
#define FUN_IN(fmt, args...)
#define FUN_OUT(fmt, args...)
#endif

#if CRYPTO_DEBUG
#define PRT_DBG(fmt, args...)             printf("%s():line[%d]"fmt"\n", __func__, __LINE__, ##args)/*  */
#else
#define PRT_DBG(fmt, args...)
#endif

#if CRYPTO_ERROR
#define PRT_ERR(fmt, args...)                                                               \
    do                                                                                          \
{                                                                                           \
    printf("\033[5;41;32m [ERROR] %s ---> %s ():line[%d]:\033[0m\n", __FILE__, __func__, __LINE__);      \
    printf(" "fmt, ##args);                                                                 \
}while(0)    /* */
#else
#define PRT_ERR(fmt, args...)
#endif

#ifdef BSRREADER_DEBUG
#define DEBUG_PRINT(x...)   printf("BSREADER:" x)
#else
#define DEBUG_PRINT(x...)    do{} while(0)
#endif


static fifo_t* myFIFO[MAX_BS_READER_SOURCE_STREAM];

static bsreader_init_data_t G_bsreader_init_data;
static int G_ready = 0;
static pthread_mutex_t G_all_mutex;

//reader thread id
static pthread_t G_main_thread_id = 0;
static int G_force_main_thread_quit = 0;

int bsreader_reset(int stream);
static int create_working_thread(void);
static int cancel_working_thread (void);
static void * bsreader_reader_thread_func(void * arg);

static void inline lock_all(void)
{
    pthread_mutex_lock(&G_all_mutex);
}

static void inline unlock_all(void)
{
    pthread_mutex_unlock(&G_all_mutex);
}
#if 0
static int set_realtime_schedule(pthread_t thread_id)
{
    struct sched_param param;
    int policy = SCHED_RR;
    int priority = 90;
    if (!thread_id)
        return -1;
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    if (pthread_setschedparam(thread_id, policy, &param) < 0)
        perror("pthread_setschedparam");
    pthread_getschedparam(thread_id, &policy, &param);
    if (param.sched_priority != priority)
        return -1;
    return 0;
}
#endif

int bsreader_init(bsreader_init_data_t * bsinit)
{
    /* data check*/
    if (bsinit == NULL) 
        return -1;

    if (G_ready) {
        printf("bsreader not closed, cannot reinit\n");
        return -1;
    }

    G_bsreader_init_data = *bsinit;

    //init mux
    pthread_mutex_init(&G_all_mutex, NULL);
    G_ready = 0;
    return 0;
}

/* allocate memory and do data initialization, threads initialization, also call reset write/read pointers */
int bsreader_open(void)
{
    u32 max_entry_num = 0;
    int ret = 0;

    FUN_IN("MAX STREAM [%d]\n", MAX_BS_READER_SOURCE_STREAM);

    lock_all();
    max_entry_num = G_bsreader_init_data.max_buffered_frame_num;
    myFIFO[0] = fifo_create(0, sizeof(bs_info_t), max_entry_num);
    
    //create thread after all other initialization is done
    if (create_working_thread() < 0) {
        printf("create working thread error \n");
        ret = -1;
        goto error_catch;
    }

    G_ready = 1;

    unlock_all();

    return 0;

error_catch:
    cancel_working_thread();
    unlock_all();
    printf("bsreader opened.\n");
    return ret;
}
static int cancel_working_thread (void)
{
    int ret;
    if (!G_main_thread_id)
        return 0;

    G_force_main_thread_quit = 1;
    ret = pthread_join(G_main_thread_id, NULL);
    if (ret < 0) {
        perror("pthread_cancel bsreader main");
        return -1;
    }
    G_main_thread_id = 0;
    return 0;
}
/* create a main thread reading data and four threads for four buffer */
static int create_working_thread(void)
{
    pthread_t tid;
    int ret;
    G_force_main_thread_quit = 0;

    ret = pthread_create(&tid, NULL, bsreader_reader_thread_func, NULL);
    if (ret != 0) {
        perror("main thread create failed");
        return -1;
    }
    G_main_thread_id = tid;
#if 0	

    if (set_realtime_schedule(G_main_thread_id) < 0) {
        printf("set realtime schedule error \n");
        return -1;
    }
#endif
    return 0;
}

/* free all resources, close all threads */
int bsreader_close(void)
{
    int ret = 0;

    lock_all();
    //TODO, possible release
    if (!G_ready) {
        printf("has not opened, cannot close\n");
        return -1;
    }

    if (cancel_working_thread() < 0) {
        perror("err:pthread_cancel bsreader main");
        return -1;
    } else {
        DEBUG_PRINT("main working thread canceled \n");
    }

    fifo_close(myFIFO[0]);

    G_ready = 0;
    unlock_all();

    pthread_mutex_destroy(&G_all_mutex);

    return ret;
}

int bsreader_get_one_frame(int stream, bsreader_frame_info_t * info)
{
    if (!G_ready) {
        return -1;
    }

    return fifo_read_one_packet(myFIFO[0], (u8 *)&info->bs_info, 
            &info->frame_addr, &info->frame_size);
}
/* dispatch frame , with ring buffer full handling  */
int dispatch_and_copy_one_frame(bsreader_frame_info_t * bs_info)
{
    int ret;
    DEBUG_PRINT("dispatch_and_copy_one_frame \n");
    ret = fifo_write_one_packet(myFIFO[0],
            (u8 *)&bs_info->bs_info, bs_info->frame_addr, bs_info->frame_size);

    return ret;
}

static void * bsreader_reader_thread_func(void * arg)
{
    bsreader_frame_info_t frame_info;
    int ret = 0;

    PRT_DBG("->enter bsreader reader main loop\n");

    //从 fifo 中读取数据并执行回调
    while (!G_force_main_thread_quit) {
        if (bsreader_get_one_frame(0, &frame_info) < 0) {
            printf("bs reader gets frame error\n");
            continue;
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
        //(*g_stSpaccParam.apfnCB[pstRespVector->io.encrypt])((MESSAGE_S *)*(((uint32_t *)pstRespVector->io.dst) + 4));
        frame_info.bs_info.cb((MESSAGE_S *)frame_info.bs_info.data);
    }
    PRT_DBG("->quit bsreader reader main loop\n");

    return NULL;
}


int bsreader_reset(int stream)
{
    if (!G_ready) {
        return -1;
    }
    printf("bsreader_reset\n");
    fifo_flush(myFIFO[stream]);

    return 0;
}
//Not implemented
int bsreader_unblock_reader(int stream)
{
    return 0;
}




