/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, xxx.
 *       Filename:  bsreader.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  09/25/2019 01:05:44 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), 544088192@qq.com
 *   Organization:  xxx
 *
 * =====================================================================================
 */

#ifndef __BSREADER_LIB_H__
#define __BSREADER_LIB_H__

#define MAX_BS_READER_SOURCE_STREAM     2

typedef unsigned int u32;
typedef unsigned char u8;

typedef struct {
    int mfd;
    int sfd;
    int crypto_type;
    unsigned char dir;
    u32 flag; 
    //call back
    void (*p)(void *) cb;
    //data of cb
    void *data;
} bs_info_t;


/* any frame stored in bsreader must has continuous memory */
typedef struct bsreader_frame_info_s {
    bs_info_t bs_info;	/* orignal information got from dsp */
    u8	*frame_addr; /* the actual address in stream buffer */
    u8	*src;
    u32	frame_size;	 /* the actual output frame size , including CAVLC */
} bsreader_frame_info_t;	

typedef struct bsreader_init_data_s {
    //u32 max_stream_num;								/* should be no more than 2*/
    //u32 ring_buf_size[MAX_BS_READER_SOURCE_STREAM];
    u32 max_buffered_frame_num;
    u8	policy;				/* 0:default drop old when ring buffer full,  1: drop new when ring buffer full*/
    u8	dispatcher_thread_priority;	 /*the dispatcher thread RT priority */		
    u8	reader_threads_priority;	/* the four reader threads' thread RT priority */
}bsreader_init_data_t;


/* set init parameters, do nothing real*/
int bsreader_init(bsreader_init_data_t * bsinit);

/* create mutex and cond, allocate memory and do data initialization, threads initialization, also call reset write/read pointers */
int bsreader_open(void);

/* free all resources, close all threads, destroy all mutex and cond */
int bsreader_close(void);

/* get one frame out of the stream
   after the frame is read out, the frame still stays in ring buffer, until next bsreader_get_one_frame will invalidate it.
   */
int bsreader_get_one_frame(int stream, bsreader_frame_info_t * info);

/* flush (discard) and reset all data in ring buf (can flush selected ringbuf), 
   do not allocate or free ring buffer memory.
   */
int bsreader_reset(int stream);
int bsreader_unblock_reader(int stream);

int dispatch_and_copy_one_frame(bsreader_frame_info_t * bs_info);


#endif // __BSREADER_LIB_H__



