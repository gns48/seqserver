/**
 * @file   seqserver.c
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue May 28 18:29:06 2019
 * 
 * @brief  main module
 * 
 * 
 */

#include "seqserver.h"

static const char PROMPT[] = "> ";

// timer callback, called each 200 ms for establ;ished connection
// after all commands have been parsed
static void client_timeout_cb(evutil_socket_t fd, short event, void *user_data) {
    struct client_data *cdata = (struct client_data*)user_data;
    if(!cdata->command_mode) {
        for(int i = 0; i < 3; i++) {
            if(0 == (cdata->count[i] && cdata->step[i])) continue;
            if(cdata->binary_mode) bufferevent_write(cdata->bev, &cdata->count[i], sizeof(uint64_t));
            else {
                char buf[32];
                int rv = snprintf(buf, sizeof(buf), "%lu ", cdata->count[i]);
                bufferevent_write(cdata->bev, buf, rv);
            }
            cdata->count[i] += cdata->step[i];
        }
    }
}

// command parser, easy state machine
static command_t parse_command(char* line, struct client_data *cdata) {
    char *p, *token, *saveptr;
    int index;
    uint64_t init, step;
    state_t state = ST_0;
    command_t cmd = CMD_ERR;
    int binary_mode = 0;
    
    for (p = line; ; p = NULL) {
        token = strtok_r(p, "\x20\x09", &saveptr);
        if(!token) break;
        if(strcmp(token, "seq1") ==0) {
            if(state != ST_0) {
                state = ST_ERR;
                break;
            }
            index = 0;
            state = ST_INIT;
        }
        else if(strcmp(token, "seq2") ==0) {
             if(state != ST_0) {
                state = ST_ERR;
                break;
            }
            index = 1;
            state = ST_INIT;
        }
        else if(strcmp(token, "seq3") == 0) {
             if(state != ST_0) {
                state = ST_ERR;
                break;
            }
            index = 2;
            state = ST_INIT;
        }
        else if(strncmp(token, "export", strlen(token)) == 0) {
             if(state != ST_0) {
                state = ST_ERR;
                break;
            }
            state = ST_EXP;
        }
        else if(state == ST_INIT) {
            if(0 == sscanf(token, "%lu", &init)) {
                state = ST_ERR;
                break;
            }
            else state = ST_STEP;
        }
        else if(state == ST_STEP) {
            if(0 == sscanf(token, "%lu", &step)) {
                state = ST_ERR;
                break;
            }
            else state = ST_END, cmd = CMD_SET;
        }
        else if(state == ST_EXP) {
            if(strncmp(token, "seq", strlen(token)) == 0) {
                state = ST_END;
                binary_mode = 0;
                cmd = CMD_WRITE;
            }
            else if(strncmp(token, "bin", strlen(token)) == 0) {
                state = ST_END;
                binary_mode = 1;
                cmd = CMD_WRITE;
            }
            else state = ST_ERR;
            break;
        }
    } // end for
    if(state == ST_END) {
        if(cmd == CMD_WRITE) cdata->binary_mode = binary_mode;
        else if(cmd == CMD_SET) {
            cdata->count[index] = init;
            cdata->step[index] = step;   
        }
    }
    return cmd;
}

// write data from client callback, called when there are some data to send available
static void client_write_cb(struct bufferevent *bev, void *user_data) {
    struct client_data *cdata = (struct client_data*)user_data;
    printf("%s: %p\n", __func__, user_data);
    if(cdata->exit_mode) {
        // disable callbacks
        event_del(&cdata->timeout);
        bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
        bufferevent_free(bev);
        free(cdata);
        // exit from loop
    }
    else bufferevent_flush(bev, EV_WRITE, BEV_NORMAL);
}

// read data from client callback.
static void client_read_cb(struct bufferevent *bev, void *user_data) {
    struct client_data *cdata = (struct client_data*)user_data;
    struct evbuffer *src;
    size_t len = 0;

    printf("%s: %p\n", __func__, user_data);
    
    src = bufferevent_get_input(bev);
    len = evbuffer_get_length(src);
    if(len) {
        char *line = evbuffer_readln(src, NULL, EVBUFFER_EOL_ANY);
        if(line) {
            if(cdata->command_mode) {
                command_t cmd = parse_command(line, cdata);
                free(line);
                if(cmd == CMD_ERR) {
                    static const char* err = "Bad command\n";
                    bufferevent_write(bev, err, strlen(err)); 
                }
                else if(cmd == CMD_WRITE) {
                    int any_defined = 0;
                    for(int i = 0; i < 3; i++) 
                        any_defined |= (cdata->count[i] && cdata->step[i]);
                    if(!any_defined) {
                        static const char* err = "No sequences defined!\n";
                        bufferevent_write(bev, err, strlen(err));
                    }
                    else { // start timer
                        cdata->command_mode = 0;
                        event_assign(&cdata->timeout, bufferevent_get_base(bev),
                                     -1, EV_PERSIST, client_timeout_cb, (void*)cdata);
                        cdata->tv.tv_sec = 0;
                        cdata->tv.tv_usec = 100;
                        event_add(&cdata->timeout, &cdata->tv);
                    }
                }
                if(cdata->command_mode) bufferevent_write(bev, PROMPT, sizeof(PROMPT)-1);
            }
            else evbuffer_drain(src, len);
        }
        else { // looks like control-C or control-D character sent
            cdata->exit_mode = 1;
        }
    }
}

// error processing callback
static void client_event_cb(struct bufferevent *bev, short events, void *user_data) {
    struct client_data *cdata = (struct client_data*)user_data;
    puts(__func__);
    if(events & BEV_EVENT_ERROR) {
        perror("connection error");
        bufferevent_free(bev);
        free(cdata);
    }
    else if(events & BEV_EVENT_EOF) {
        fputs("Client disconnected", stderr);
        bufferevent_free(bev);
        free(cdata);
    }
}

// TERM signal handler 
static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	struct timeval delay = {1, 0};

	printf("Caught an interrupt signal; exiting cleanly in one seconds.\n");
	event_base_loopexit(base, &delay);
}

// listener callback, called by listener for each new connection
static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                        struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = user_data;
	struct bufferevent *bev;
    struct client_data *cdata;

    cdata = calloc(1, sizeof(struct client_data));
    if(!cdata) {
        fprintf(stderr, "Error allocating client!");
		event_base_loopbreak(base);
		return;
    }

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		fprintf(stderr, "Error constructing bufferevent!");
        free(cdata);
		event_base_loopbreak(base);
		return;
	}

    cdata->command_mode = 1;
    cdata->bev = bev;
    printf("%s: %p\n", __func__, (void*)cdata);
	bufferevent_setcb(bev, client_read_cb, client_write_cb, client_event_cb, (void*)cdata);
	bufferevent_enable(bev, EV_WRITE | EV_READ);
    bufferevent_write(bev, PROMPT, sizeof(PROMPT)-1);
}

int main(int ac, char **av) {
	struct event_base *base;
	struct evconnlistener *listener;
	struct event *signal_event;
	struct sockaddr_in sin;
    uint16_t port;

    if(ac < 2) {
        fprintf(stderr, "Usage: %s <port>\n", av[0]);
        return EINVAL;
    }

    if(1 != sscanf(av[1], "%hu", &port)) {
        fprintf(stderr, "Bad port specified: %s\n", av[1]);
        fprintf(stderr, "Usage: %s <port>\n", av[0]);
        return EINVAL;
    }

    // allocate event base
    base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return ECANCELED;
	}

    // setup listening socket
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
                                       LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
                                       (struct sockaddr*)&sin, sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
        event_base_free(base);
		return ECANCELED;
	}

    // setup TERM signal handler
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);
	if (!signal_event || event_add(signal_event, NULL) < 0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
        evconnlistener_free(listener);
        event_base_free(base);
		return ECANCELED;
	}

    // main event loop
	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);
    
	return 0;
}



