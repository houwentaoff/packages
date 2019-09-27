/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, xxx.
 *       Filename:  fifo.h
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  09/25/2019 01:05:58 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), 544088192@qq.com
 *   Organization:  xxx
 *
 * =====================================================================================
 */

#ifndef _FIFO_H
#define _FIFO_H

#define OneCircle     1
#define ZeroCircle    0
#define NOMORENUM     0
#define WriteSuccess  1
#define ReadSuccess   1

#define FIFO_IS_FULL  0
#define FIFO_IS_EMPTY 0

typedef unsigned int u32;
typedef unsigned char u8;

typedef struct packet_info_s
{
    u8    *packet_addr;//L2 ADDR
    u32	packet_size;//encrypto data size
    u32	entry_state;
} packet_info_t;

typedef struct fifo_s
{	
    // buffer related
    //u8    			*buffer_start_addr;
    //u8    			*buffer_end_addr;
    //u32				buffer_total_size;
    // entry related
    packet_info_t		*packet_info_entries;
    u32				max_entry_num;
    int				first_entry_index;
    int				last_entry_index;
    // header related
    u8				*header_entries;//
    u32				header_size;
    // fifo core
    pthread_mutex_t	mutex;
    pthread_cond_t	cond;

    // buffer related
    //not use
    //u8    			*addr_for_next_write;
    //u32				buffer_used_size;

    // entry related
    int				entry_index_for_next_read;
    int				entry_index_for_next_write;
    int				entry_index_being_used;
    u32				used_entry_num;//Sean ?
    // others
    u32				non_block_flag;
} fifo_t;

fifo_t* fifo_create(u32 buffer_size, u32 header_size, u32 max_packet_num);
void fifo_close(fifo_t *fifo);

int fifo_write_one_packet(fifo_t *fifo, u8 *header, u8 *packet_addr, u32 packet_size);
int fifo_read_one_packet(fifo_t *fifo, u8 *header, u8 **packet_addr, u32 *packet_size);
int fifo_flush(fifo_t *fifo);


#endif

