#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
# include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello client!\n";

static const int PORT = 8080;

static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);

static void conn_readcb(struct bufferevent *, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

const int listenBacklog = 10;

int main(int argc, char **argv){
    struct event_base *base;
    struct evconnlistener *listener;
    struct event *signal_event;
    struct sockaddr_in sin;
#ifdef WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif
	printf("This is the server\n");

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    //连接侦听器
    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, listenBacklog,
        (struct sockaddr*)&sin,
        sizeof(sin));

    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;
    }

    //注册普通的信号事件
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

    if (!signal_event || event_add(signal_event, NULL)<0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return 1;
    }

    //开始事件循环
    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}

//接收新连接的回调函数
static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data){
	struct event_base *base = (struct event_base *) user_data;
	struct bufferevent *bev;

    //为每个客户端分配一个bufferevent,用于后续的IO操作
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    //bufferevent 不启用读取和写入事件,则bufferevent不会进行读取和写入
    bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);//设置可读,可写,出错的回调函数

    bufferevent_enable(bev, EV_WRITE|EV_PERSIST|EV_READ);

    //发送hello-world到客户端
    bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

char acn[4][20] = {"user:aaa,pwd:aaa\n","user:bbb,pwd:bbb\n","user:ccc,pwd:ccc\n","user:ddd,pwd:ddd\n"};
char usr[4][20] = {0};
char board[] = {"___|___|___\n___|___|___\n   |   |   \n\n"};
char tmp[] = {"___|___|___\n___|___|___\n   |   |   \n\n"};
int mark[3][3];
int status[4] = {0};
int match[2] = {-1,-1};
int chs=-1,gus=-1,now=-1,cnt=0;

static void conn_readcb(struct bufferevent *bev, void *user_data){
    //fprintf(stdout,"conn_readcb() function called!\r\n");
	struct evbuffer* input = bufferevent_get_input(bev);
    char szBuffer[32] = {0};
	
    evbuffer_remove(input,szBuffer,32);
	printf("%s",szBuffer);
	int i,id;	
	id = bufferevent_getfd(bev) - 7;
	
	if (status[id] == 0){
		char name[20] = {0};
		for (i=0;i<4;i++){
			if (strcmp(szBuffer,acn[i]) == 0){
				status[id] = 1;	
				strncpy(name,szBuffer+5,3);
				name[3] = '\n';
				name[4] = '\0';
				strcpy(usr[id],name);
				bufferevent_write(bev, "Login success!\n", 15);
			}
		}
	}
	
	if (status[id] == 0 && strcmp("login\n",szBuffer) != 0){
		bufferevent_write(bev, "Please login first!\n", 20);
	}
	else if (strcmp("login\n",szBuffer) == 0){
		if (status[id] == 0){
			bufferevent_write(bev, "Please type your username and passwd!\n", 38);
		}
		else
			bufferevent_write(bev, "You are logged in!\n", 19);
	}

	if (status[id] == 1){
		if (strcmp("ls\n",szBuffer) == 0){
			for (i=0;i<4;i++)
				if (status[i] >= 1)
					bufferevent_write(bev, usr[i], strlen(usr[i]));
		}
		else if (strcmp("logout\n",szBuffer) == 0){
			status[id] = 0;
			memset(usr[id],'\0',sizeof(usr[id]));
			bufferevent_write(bev, "Logout success!\n", 16);
		}
		else if (strncmp("play",szBuffer,4) == 0){
			int op_id=-1;
			char op[10];
			strcpy(op,szBuffer+5);
			for (i=0;i<4;i++){
				if (strcmp(usr[i],op) == 0){
					op_id = i;
					break;
				}
			}
			if (op_id == id)
				bufferevent_write(bev, "You can't play with yourself!\n", 30);
			else if (op_id == -1)
				bufferevent_write(bev, "Sorry, can't find this user!\n", 29);
			else if (status[op_id] == 2)
				bufferevent_write(bev, "Sorry, the user is in the game!\n", 32);
			else {
				bufferevent_write(bev, "Waiting for the opponent to accept!\n", 36);
				char msg[30];
				strcpy(msg,usr[id]);
				msg[3] = '\0';
				strcat(msg," invite you!\n");
				write(op_id+7,msg,strlen(msg));
			}
		}
		else if (strncmp("yes",szBuffer+4,3) == 0){
			char p1[5],p2[5],msg[30];	// p1 invite p2
			int op_id=-1;
			strncpy(p1,szBuffer,3);
			p1[3] = '\0';
			strcpy(p2,usr[id]);
			for (i=0;i<4;i++){
				if (strncmp(usr[i],p1,3) == 0){
					op_id = i;
					break;
				}
			}
			write(op_id+7, "The opponent accept!\n", 21);
			strcpy(msg,"Game start!\n\n");
			write(id+7, msg, strlen(msg));
			write(op_id+7, msg, strlen(msg));
			match[0] = id;
			match[1] = op_id;
			write(id+7, "Choose a number to be guessed!\n", 32);
			write(op_id+7, "Guess the number is even(0) or odd(1)!\n", 39);
		}
	}
	if (match[0] != -1 && match[1] != -1){
		if (status[id] != 2 && (chs == -1 || gus == -1)){
			if (strncmp(szBuffer, "choose", 6) == 0)
				chs = atoi(szBuffer+7);	
			else if (strncmp(szBuffer, "guess", 5) == 0)
				gus = atoi(szBuffer+6);
		}
		if (chs != -1 && gus != -1){
			if (chs % 2 == 0){
				if (gus == 0){
					now = match[1];
					write(match[1]+7,"You are sente!\n\n", 16);
					write(match[0]+7,"You are gote!\n\n", 15);
				}
				else {
					now = match[0];
					write(match[0]+7, "You are sente!\n\n", 16);
					write(match[1]+7, "You are gote!\n\n", 15);
				}
			}
			else {
				if (gus == 1){
					now = match[1];
					write(match[1]+7, "You are sente!\n\n", 16);
					write(match[0]+7, "You are gote!\n\n", 15);
				}
				else {
					now = match[0];
					write(match[0]+7, "You are sente!\n\n", 16);
					write(match[1]+7, "You are gote!\n\n", 15);
				}
			}
			status[match[0]] = 2;
			status[match[1]] = 2;
			write(match[0]+7, board, strlen(board));
			write(match[1]+7, board, strlen(board));
			chs = -1;
			gus = -1;
			memset(mark, 0, sizeof(mark));
		}
		if (status[id] == 2 && now != -1){
			if (strncmp(szBuffer, "go", 2) == 0){
				if (now != id){
					write(id+7, "Not your turn!\n\n", 16);
					write(id+7, board, strlen(board));
				}
				else {
					int x,y;
					x = szBuffer[3] - '0';
					y = szBuffer[5] - '0';
					if (mark[x][y] != 0){
						write(id+7,"You can't go here, choose again!\n\n", 34);
						write(id+7, board, strlen(board));
					}
					else if (x > 2 || y > 2){
						write(id+7, "Wrong position, choose again!\n\n", 31);
						write(id+7, board, strlen(board));
					}
					else {
						mark[x][y] = now+1;
						cnt++;
						int pos;
						pos = 12 * x + y * 4 + 1;
						if (now == match[0])
							board[pos] = 'O';
						else
							board[pos] = 'X';

						if (now == match[0]){
							write(match[1]+7, "It's your turn!\n\n", 17);
							write(match[1]+7, board, strlen(board));
							write(match[0]+7, "Done, wait!\n\n", 13);
							write(match[0]+7, board, strlen(board));
							now = match[1];
						}
						else {
							write(match[0]+7, "It's your turn!\n\n", 17);
							write(match[0]+7, board, strlen(board));
							write(match[1]+7, "Done, wait!\n\n", 13);
							write(match[1]+7, board, strlen(board));
							now = match[0];
						}
					}

					if (cnt > 2){
						int j;
						int x = (now == match[0] ? match[1] : match[0]);
						int flag=0;
						x++;
						if (mark[0][0] == x && mark[0][1] == x && mark[0][2] == x)
							flag = 1;
						else if (mark[1][0] == x && mark[1][1] == x && mark[1][2] == x)
							flag = 1;
						else if (mark[2][0] == x && mark[2][1] == x && mark[2][2] == x)
							flag = 1;
						else if (mark[0][0] == x && mark[1][0] == x && mark[2][0] == x)
							flag = 1;
						else if (mark[0][1] == x && mark[1][1] == x && mark[2][1] == x)
							flag = 1;
						else if (mark[0][2] == x && mark[1][2] == x && mark[2][2] == x)
							flag = 1;
						else if (mark[0][0] == x && mark[1][1] == x && mark[2][2] == x)
							flag = 1;
						else if (mark[0][2] == x && mark[1][1] == x && mark[2][0] == x)
							flag = 1;

						if (flag == 1){
							if (now == match[1]){
								write(match[0]+7, "You win!\nLeave the game...\n", 27);
								write(match[1]+7, "You lose!\nLeave the game...\n", 28);
							}
							else {
								write(match[1]+7, "You win!\nLeave the game...\n", 27);
								write(match[0]+7, "You lose!\nLeave the game...\n", 28);
							}
							flag = 0;
							memset(mark, 0, sizeof(mark));
							strcpy(board,tmp);
							cnt = 0;
							now = -1;
							status[match[0]] = 1;
							status[match[1]] = 1;
							match[0] = match[1] = -1;
						}
						else {
							if (cnt == 9){
								write(match[0]+7, "Draw!\nLeave the game...\n", 24);
								write(match[1]+7, "Draw!\nLeave the game...\n", 24);
								memset(mark, 0, sizeof(mark));
								memset(board, 0, sizeof(board));
								strcpy(board,tmp);
								cnt = 0;
								now = -1;
								status[match[0]] = 1;
								status[match[1]] = 1;
								match[0] = match[1] = -1;
							}
						}
					}
				}
			}
			else if (strncmp(szBuffer, "ff", 2) == 0){
				if (match[0] == id){
					write(match[0]+7, "sp4, you lose!\nLeave the game...\n", 34);
					write(match[1]+7, "You win!\nLeave the game...\n", 28);
				}
				else {
					write(match[1]+7, "sp4, you lose!\nLeave the game...\n", 34);
					write(match[0]+7, "You win!\nLeave the game...\n", 28);
				}
				memset(mark, 0, sizeof(mark));
				strcpy(board,tmp);
				cnt = 0;
				now = -1;
				status[match[0]] = 1;
				status[match[1]] = 1;
				match[0] = match[1] = -1;
			}
		}
	}
}

static void conn_writecb(struct bufferevent *bev, void *user_data){
    //fprintf(stdout,"conn_writecb() function called!\r\n");

    struct evbuffer *output = bufferevent_get_output(bev);

    //解析发送的字符串
    char szBuffer[32] = {0};
    if (evbuffer_get_length(output) == 0) {
        printf("flushed answer\n");

        //后续还需要使用bufferevent,此处不释放资源
        //bufferevent_free(bev);
    }
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data){
    //fprintf(stdout,"conn_eventcb() function called!\r\n");
    if (events & BEV_EVENT_EOF) 
    {
        printf("Connection closed.\n");
    } 
    else if (events & BEV_EVENT_ERROR) 
    {
        printf("Got an error on the connection: %s\n",
        strerror(errno));/*XXX win32*/
    }
    /* None of the other events can happen here, since we haven't enabled * timeouts */
    bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data){
    struct event_base *base = user_data;
    struct timeval delay = { 0 , 0 };

    //printf("Caught an interrupt signal; exiting cleanly in one second.\n");

    event_base_loopexit(base, &delay);
}
