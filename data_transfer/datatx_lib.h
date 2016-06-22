/*
****************************************************************************
*
** \file      datatx_lib.h
**
** \version   $Id$
**
** \brief     
**
** \attention THIS SAMPLE CODE IS PROVIDED AS IS. GOFORTUNE SEMICONDUCTOR
**            ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR 
**            OMMISSIONS.
**
** (C) Copyright 2012-2013 by GOKE MICROELECTRONICS CO.,LTD
**
****************************************************************************
*/


#ifndef _DATATX_LIB_H_
#define _DATATX_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum trans_method_s {
	TRANS_METHOD_NONE = 0,
	TRANS_METHOD_NFS,
	TRANS_METHOD_TCP,
	TRANS_METHOD_USB,
	TRANS_METHOD_STDOUT,
	TRANS_METHOD_UNDEFINED,
} trans_method_t;

int transfer_init(int transfer_method);
int transfer_deinit(int transfer_method);
int transfer_open(const char *name, int transfer_method, int port);
int transfer_write(int fd, void *data, int bytes, int transfer_method);
int transfer_close(int fd, int transfer_method);

#ifdef __cplusplus
}
#endif


#endif
