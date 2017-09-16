// 功能：实现简单的web服务器功能，能同时响应多个浏览器的请求：
//       1、如果该文件存在，则在浏览器上显示该文件；
//       2、如果文件不存在，则返回404-file not found页面
//       3、只支持GET、HEAD方法
// HTTP1.1 与 1.0不同，默认是持续连接的(keep-alive)

#include <Winsock2.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>     // 目录头文件
#pragma comment(lib,"Ws2_32.lib")

// http 默认端口是80，如果80端口被占用那么改个端口即可
#define DEFAULT_PORT 8080
#define BUF_LENGTH 1024
#define MIN_BUF 256
#define USER_ERROR -1
#define SERVER "Server: csr_http1.1\r\n"

int file_not_found(SOCKET sAccept);
int file_ok(SOCKET sAccept, long flen, char* extension);
int send_file(SOCKET sAccept, FILE *resource);
int send_not_found(SOCKET sAccept);
char* file_type(char* arg);

DWORD WINAPI SimpleHTTPServer(LPVOID lparam)
{
    SOCKET sAccept = (SOCKET)(LPVOID)lparam;
    char recv_buf[BUF_LENGTH]; 
    char method[MIN_BUF];
    char url[MIN_BUF];
    char path[_MAX_PATH];
    int i, j;

    // 不清空可能出现的现象：输出乱码、换台机器乱码还各不相同
    // 原因：不清空会输出遇到 '\0'字符为止，所以前面的不是'\0' 也会一起输出
    memset(recv_buf,0,sizeof(recv_buf));// 缓存清0，每次操作前都要记得清缓存，养成习惯；
    if (recv(sAccept,recv_buf,sizeof(recv_buf),0) == SOCKET_ERROR)   //接收错误
    {
        printf("recv() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }       
    else
        printf("recv data from client:%s\n",recv_buf); //接收成功，打印请求报文

    //处理接收数据
    i = 0; j = 0;
    // 取出第一个单词，一般为HEAD、GET、POST
    while (!(' ' == recv_buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = recv_buf[j];
        i++; j++;
    }
    method[i] = '\0';   // 结束符，这里也是初学者很容易忽视的地方

    // 如果不是GET或HEAD方法，则直接断开本次连接
    // 如果想做的规范些可以返回浏览器一个501未实现的报头和页面
    if (stricmp(method, "GET") && stricmp(method, "HEAD"))
    {
        closesocket(sAccept); //释放连接套接字，结束与该客户的通信
        printf("not get or head method.\nclose ok.\n");
        printf("***********************\n\n\n\n");
        return USER_ERROR;
    }
    printf("method: %s\n", method);

    // 提取出第二个单词(url文件路径，空格结束)，并把'/'改为windows下的路径分隔符'\'
    // 这里只考虑静态请求(比如url中出现'?'表示非静态，需要调用CGI脚本，'?'后面的字符串表示参数，多个参数用'+'隔开
    // 例如：www.csr.com/cgi_bin/cgi?arg1+arg2 该方法有时也叫查询，早期常用于搜索)
    i = 0;
    while ((' ' == recv_buf[j]) && (j < sizeof(recv_buf)))
        j++;
    while (!(' ' == recv_buf[j]) && (i < sizeof(recv_buf) - 1) && (j < sizeof(recv_buf)))
    {
        if (recv_buf[j] == '/')
            url[i] = '\\';
        else if(recv_buf[j] == ' ')
            break;
        else
            url[i] = recv_buf[j];
        i++; j++;
    }
    url[i] = '\0';
    printf("url: %s\n",url);

    // 将请求的url路径转换为本地路径
    _getcwd(path,_MAX_PATH);
    strcat(path,url);
    printf("path: %s\n",path);

    // 打开本地路径下的文件，网络传输中用r文本方式打开会出错
    FILE *resource = fopen(path,"rb+");///*******************///

    // 没有该文件则发送一个简单的404-file not found的html页面，并断开本次连接
    if(resource==NULL)
    {
        file_not_found(sAccept);
        // 如果method是GET，则发送自定义的file not found页面
        if(0 == stricmp(method, "GET"))
            send_not_found(sAccept);

        closesocket(sAccept); //释放连接套接字，结束与该客户的通信
        printf("file not found.\nclose ok.\n");
        printf("***********************\n\n\n\n");
        return USER_ERROR;
    }

    // 求出文件长度，记得重置文件指针到文件头
    fseek(resource,0,SEEK_SET);//设置文件指针stream的位置。
							//如果执行成功，stream将指向以SEEK_SET为基准，偏移0（指针偏移量）个字节的位置，函数返回0。
							//如果执行失败(比如offset超过文件自身大小)，则不改变stream指向的位置，函数返回一个非0值。
    fseek(resource,0,SEEK_END);
    long flen=ftell(resource);//得到文件位置指针当前位置相对于文件首的偏移字节数
    printf("file length: %ld\n", flen);
    fseek(resource,0,SEEK_SET);

	char* extension = file_type(url);
	file_ok(sAccept,flen,extension);   // 发送200 OK HEAD

    // 如果是GET方法则发送请求的资源
    if(0 == stricmp(method, "GET"))
    {
        if(0 == send_file(sAccept, resource))
            printf("file send ok.\n");
        else
            printf("file send fail.\n");
    }
    fclose(resource);

    closesocket(sAccept); //释放连接套接字，结束与该客户的通信
    printf("close ok.\n");
    printf("***********************\n\n\n\n");

    return 0;

}

// 发送404 file_not_found报头
int file_not_found(SOCKET sAccept)
{
    char send_buf[MIN_BUF]; 
//  time_t timep;   
//  time(&timep);
    sprintf(send_buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
//  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
//  send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, SERVER);
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "Content-Type: text/html\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

//判断文件类型，即文件后缀名，.html,.jpg,.css等；
char* file_type(char* arg) {
	char * temp;         
	if ((temp = strrchr(arg, '.')) != NULL) {   //查找字符在指定字符串中从左面开始的最后一次出现的位置 
		return temp + 1;
	}
	return "";          
}

// 发送200 ok报头
int file_ok(SOCKET sAccept, long flen, char* extension)
{
    char send_buf[MIN_BUF]; 
//  time_t timep;
//  time(&timep);
    sprintf(send_buf, "HTTP/1.1 200 OK\r\n");//把格式化的数据写入某个字符串缓冲区。
    send(sAccept, send_buf, strlen(send_buf), 0);//用于向一个已经连接的socket发送数据，
												 //如果无错误，返回值为所发送数据的总数，否则返回SOCKET_ERROR。
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
//  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
//  send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, SERVER);//设置报头Server
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, "Content-Length: %ld\r\n", flen);
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

	char* content_type = "Content-Type: text/plain\r\n";

	if (strcmp(extension, "html") == 0) {    //发送内容为html
		content_type = "Content-Type: text/html\r\n";
	}
	if (strcmp(extension, "htm") == 0) {    //发送内容为html
		content_type = "Content-Type: text/htm\r\n";
	}
	if (strcmp(extension, "ico") == 0) {    //发送内容为ico
		content_type = "Content-Type: image/ico\r\n";
	}
	if (strcmp(extension, "css") == 0) {    //发送内容为css
		content_type = "Content-Type: text/css\r\n";
	}
	if (strcmp(extension, "gif") == 0) {    //发送内容为gif
		content_type = "Content-Type: image/gif\r\n";
	}
	if (strcmp(extension, "jpg") == 0) {    //发送内容为jpg
		content_type = "Content-Type: image/jpg\r\n";
	}
	if (strcmp(extension, "png") == 0) {    //发送内容为png
		content_type = "Content-Type: image/png\r\n";
	}
	if (strcmp(extension, "js") == 0) {    //发送内容为javascript
		content_type = "Content-Type: application/x-javascript\r\n";
	}
	/*if (strcmp(extension, "txt") == 0) {    //发送内容为txt
		content_type = "Content-Type: text/txt\r\n";
	}*/

    sprintf(send_buf, content_type);
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, "Transfer-Encoding: gzip\r\n");
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, "\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

// 发送自定义的file_not_found页面
int send_not_found(SOCKET sAccept)
{
    char send_buf[MIN_BUF];
    sprintf(send_buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "<BODY><h1 align='center'>404</h1><br/><h1 align='center'>找不到页面.</h1>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "</BODY></HTML>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

// 发送请求的资源
int send_file(SOCKET sAccept, FILE *resource)
{
    char send_buf[409600];
    while (1)
    {
        fread(send_buf,40950,1,resource);     //从一个文件流中读数据,从一个文件流中读数据，
											  //最多读取count个项，每个项size个字节
        if (SOCKET_ERROR == send(sAccept, send_buf, 40960, 0))
        {
            printf("send() Failed:%d\n",WSAGetLastError());
            return USER_ERROR;
        }
        if(feof(resource))//检测流上的文件结束符，如果文件结束，则返回非0值
            return 0;
    }   
}

int main()
{
    WSADATA wsaData;				//用来存储被WSAStartup函数调用后返回的Windows Sockets数据
    SOCKET sListen,sAccept;        //服务器监听套接字，连接套接字
    int serverport=DEFAULT_PORT;   //服务器端口号
    struct sockaddr_in ser,cli;   //服务器地址，客户端地址
    int iLen;					  //得到地址结构体的长度  accept函数需要

    printf("--------------------------------\n");
    printf("server is ready in listening ...\n");
    printf("--------------------------------\n");

    //第一步：加载协议栈
    if (WSAStartup(MAKEWORD(2,2),&wsaData) !=0)//判断本机版本是否相符
    {
        printf("Failed to load Winsock.\n");
        return USER_ERROR;
    }

    //第二步：创建监听套接字，用于监听客户请求
    sListen =socket(AF_INET,SOCK_STREAM,0);
    if (sListen == INVALID_SOCKET)//如果套接字创建失败则返回INVALID_SOCKET
    {
        printf("socket() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //创建服务器地址：IP+端口号
    ser.sin_family=AF_INET;					 //建立一个连接,,sin_family表示协议簇,一般用AF_INET表示TCP/IP协议.
    ser.sin_port=htons(serverport);          //服务器端口号
    ser.sin_addr.s_addr=htonl(INADDR_ANY);   //服务器IP地址，默认使用本机IP
											 //in_addr是一个联合体,用联合体就可以使用多种方式表示IP地址

    //第三步：绑定监听套接字和服务器地址
    if (bind(sListen,(LPSOCKADDR)&ser,sizeof(ser))==SOCKET_ERROR)
    {
        printf("blind() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //第五步：通过监听套接字进行监听
    if (listen(sListen,5)==SOCKET_ERROR)
    {
        printf("listen() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }
    while (1)  //循环等待客户的请求
    {
        //第六步：接受客户端的连接请求，返回与该客户建立的连接套接字
        iLen=sizeof(cli);//得到地址结构体的长度  accept函数需要
        sAccept=accept(sListen,(struct sockaddr*)&cli,&iLen);////accept阻塞 知道有新的连接
        if (sAccept==INVALID_SOCKET)
        {
            printf("accept() Failed:%d\n",WSAGetLastError());
            break;
        }
        //第七步，创建线程接受浏览器请求
        DWORD ThreadID;
        CreateThread(NULL,0,SimpleHTTPServer,(LPVOID)sAccept,0,&ThreadID);  
    }
    closesocket(sListen);
    WSACleanup();
    return 0;
}