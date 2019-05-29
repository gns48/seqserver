/**
 * @file   seqserver.h
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue May 28 16:57:51 2019
 * 
 * @brief  Основные определения и типы
 * 
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

/*
  libevent2 stuff
*/
#include <event2/event-config.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>


#ifndef __SEQSERVER_H__
#define __SEQSERVER_H__

// user state

typedef struct client_data {
    uint64_t count[3];         // current counters
    uint64_t step[3];          // current steps
    struct bufferevent *bev;   // saved own buferevent
    struct event timeout;      // own timer event
	struct timeval tv;         // timeout
    int binary_mode;           // do we send binary data?
    int command_mode;          // commands not yet parsed
    int exit_mode;
} client_data_t;

// state machine states for command parsing
typedef enum _state {ST_0, ST_INIT, ST_STEP, ST_WRITE, ST_EXP, ST_END, ST_ERR} state_t;
typedef enum _command {CMD_ERR, CMD_SET, CMD_WRITE} command_t;

#endif // #ifndef __SEQSERVER_H__







