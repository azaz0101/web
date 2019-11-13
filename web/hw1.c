#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8096

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void handle (int fd){
    int j, file_fd, buflen, len;
    long i, ret;
    char *fstr;
    static char buffer[BUFSIZE+1];

    ret = read(fd,buffer,BUFSIZE);   /* 讀取瀏覽器要求 */

    if (ret == 0 || ret == -1) {
     /* 網路連線有問題，所以結束行程 */
        exit(3);
    }
	
	printf("%s\n",buffer);

    /* 程式技巧：在讀取到的字串結尾補空字元，方便後續程式判斷結尾 */
    if (ret > 0 && ret < BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;
	
	if (!strncmp(buffer, "GET ",4)) {
    	/* 我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
    	for(i=4;i<BUFSIZE;i++) {
        	if(buffer[i] == ' ') {
            	buffer[i] = 0;
            	break;    
			}
   	 	}

    	/* 檔掉回上層目錄的路徑『..』 */
    	for (j=0;j<i-1;j++)
        	if (buffer[j] == '.' && buffer[j+1] == '.')
            	exit(3);

    	/* 當客戶端要求根目錄時讀取 index.html */
    	if (!strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ){
        	strcpy(buffer,"GET /index.html\0");
		}

    	/* 檢查客戶端所要求的檔案格式 */
    	buflen = strlen(buffer);
    	fstr = NULL;

    	for(i=0;extensions[i].ext!=0;i++) {
        	len = strlen(extensions[i].ext);
        	if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
            	fstr = extensions[i].filetype;
            	break;
        	}
    	}

    	/* 檔案格式不支援 */
    	if(fstr == 0) {
        	fstr = extensions[i-1].filetype;
    	}
	
		/* 開啟檔案 */
    	if((file_fd = open(&buffer[5],O_RDONLY)) == -1)
  			write(fd, "Failed to open file\n", 20);

    	/* 傳回瀏覽器成功碼 200 和內容的格式 */
    	sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    	write(fd,buffer,strlen(buffer));


    	/* 讀取檔案內容輸出到客戶端瀏覽器 */
    	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) {
        	write(fd,buffer,ret);
    	}
	}
	else if (!strncmp(buffer, "POST ",5)){
		char *ptr = strstr(buffer, "filename");
		char *ptr2 = NULL;
		char filename[1024];
		char path[1024] = "./tmp/";
		int cnt=0;
		ptr += 10;
		while(*ptr != '"'){
			filename[cnt] = *ptr;
			ptr++;
			cnt++;
		}
		filename[cnt] = 0;
		strcat(path,filename);
		ptr2 = strstr(ptr,"\n\r\n");
		ptr2 += 3;
		FILE *fp = fopen(path, "a+");
		while(*ptr2 != '\n' || *(ptr2+1) != '-' || *(ptr2+2) != '-'){
			fprintf(fp,"%c",*ptr2);
			ptr2++;
		}
		fclose(fp);
		write(fd,"Done\n",5);
	}
	exit(1);
}

int main(int argc, char **argv){
    int i, pid, listenfd, socketfd;
    socklen_t length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;
	
	/* 開啟網路 Socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
        exit(3);
		
    /* 網路連線設定 */
    serv_addr.sin_family = AF_INET;
    /* 使用任何在本機的對外 IP */
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* 使用 8080 Port */
    serv_addr.sin_port = htons(8080);

    /* 開啟網路監聽器 */
    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        exit(3);

    /* 開始監聽網路 */
    if (listen(listenfd,64) < 0)
        exit(3);
	
	while(1) {
        length = sizeof(cli_addr);
        /* 等待客戶端連線 */
        if ((socketfd = accept(listenfd, (struct sockaddr *) &cli_addr, &length))<0)
            exit(3);

        /* 分出子行程處理要求 */
        if ((pid = fork()) < 0) {
            exit(3);
        } 
		else {
        	if (pid == 0) { 
				/* 子行程 */
                close(listenfd);
                handle(socketfd);
            }
			else { 
				/* 父行程 */
                close(socketfd);
            }
        }
    }
	return 0;
}
