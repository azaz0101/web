#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

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

#define EVENT_BASE_SOCKET 0

static const int PORT = 8080;
const char* server_ip = "127.0.0.1";

/*事件处理回调函数*/
void event_cb(struct bufferevent* bev,short events,void* ptr){
    if(events & BEV_EVENT_CONNECTED)//连接建立成功
    {
        printf("connected to server successed!\n");
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("connect error happened!\n");
    }
}

void read_cb(struct bufferevent *bev, void *ctx){
    char msg[200] = {0};
    struct evbuffer* buf = bufferevent_get_input(bev);
    evbuffer_remove(buf,(void*)msg,200);

    printf("%s\r\n",msg);

	if (strncmp(msg+4,"invite",6) == 0){
		char rep[30] = {0};
		strncpy(rep, msg, 3);
		rep[3] = ' ';
		scanf("%s",rep+4);
		rep[strlen(rep)+1] = '\0';
		rep[strlen(rep)] = '\n';
		bufferevent_write(bev, rep, strlen(rep));
	}
	else if (strncmp(msg, "choose", 6) == 0){
		char rep[10] = {0};
		scanf("%s",rep);
		rep[strlen(rep)+1] = '\0';
		rep[strlen(rep)] = '\n';
		bufferevent_write(bev, rep, strlen(rep));
	}
	else if (strncmp(msg, "guess", 5) == 0){
		char rep[10] = {0};
		scanf("%s",rep);
		rep[strlen(rep)+1] = '\0';
		rep[strlen(rep)] = '\n';
		bufferevent_write(bev, rep, strlen(rep));
	}
}

int tcp_connect_server(const char* server_ip,int port){
    struct sockaddr_in server_addr;
    int status = -1;
    int sockfd;

    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port);  
    status = inet_aton(server_ip, &server_addr.sin_addr);
    if(0 == status){
        errno = EINVAL;
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if( sockfd == -1 )  
        return sockfd;  


    status = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) );  

    if( status == -1 ) {
        close(sockfd);
        return -1;  
    }  

    evutil_make_socket_nonblocking(sockfd);

    return sockfd;
}

void cmd_msg_cb(int fd, short events, void *arg){
    char msg[1024];
	memset(msg,'\0',sizeof(msg));
    int ret = read(fd,msg,sizeof(msg));
    if(ret < 0 )
    {
        perror("read failed");
        exit(1);
    }

    struct bufferevent* bev = (struct bufferevent*)arg;

    msg[ret] = '\0';
    //把终端消息发给服务器段
    bufferevent_write(bev,msg,ret);

    //printf("send message %s\r\n",msg);
}

int main(){
    struct event_base* base = NULL;
    struct sockaddr_in server_addr;
    struct bufferevent *bev = NULL;
    int status;
    int sockfd;

    //申请event_base对象
    base = event_base_new();
    if(!base){
        printf("event_base_new() function called error!");
    }

    //初始化server_addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port= htons(PORT);
    status = inet_aton(server_ip, &server_addr.sin_addr);

#if EVENT_BASE_SOCKET

    sockfd = tcp_connect_server(server_ip,PORT);
    bev = bufferevent_socket_new(base, sockfd,BEV_OPT_CLOSE_ON_FREE);
#else
    bev = bufferevent_socket_new(base, -1,BEV_OPT_CLOSE_ON_FREE);//第二个参数传-1,表示以后设置文件描述符

    //调用bufferevent_socket_connect函数
    bufferevent_socket_connect(bev,(struct sockaddr*)&server_addr,sizeof(server_addr));
#endif
    //监听终端的输入事件
    struct event* ev_cmd = event_new(base, STDIN_FILENO,EV_READ | EV_PERSIST, cmd_msg_cb, (void*)bev);  

    //添加终端输入事件
    event_add(ev_cmd, NULL);

    //设置bufferevent各回调函数
    bufferevent_setcb(bev,read_cb, NULL, event_cb, (void*)NULL);  

    //启用读取或者写入事件
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

    //开始事件管理器循环
    event_base_dispatch(base);

    event_base_free(base);
    return 0;

}
