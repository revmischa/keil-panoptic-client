#ifndef PANOPTIC_CLIENT_H
#define PANOPTIC_CLIENT_H

#include "int80_api.h"

#define POC_PORT_NUM 8422
#define POC_IDLE_TIMEOUT 60
#define POC_RAW_MESSAGE_QUEUE_MAX 32

typedef struct {
    char *msg;
	unsigned int msg_len;
} raw_message_t;

typedef int80_message message;

#include <RTL.h>
#include <debug.h>  // assert
#include <stdio.h>  // sprintf
#include <string.h>	// memcpy
#include <stdlib.h>	// malloc

#include "int80_api.h"
#include "common.h"
#include "main.h"


void init_poc(void);
__task void poc_task(void);

#endif
