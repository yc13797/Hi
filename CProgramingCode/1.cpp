// ���ܣ�ʵ�ּ򵥵�web���������ܣ���ͬʱ��Ӧ��������������
//       1��������ļ����ڣ��������������ʾ���ļ���
//       2������ļ������ڣ��򷵻�404-file not foundҳ��
//       3��ֻ֧��GET��HEAD����
// HTTP1.1 �� 1.0��ͬ��Ĭ���ǳ������ӵ�(keep-alive)

#include <Winsock2.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>     // Ŀ¼ͷ�ļ�
#pragma comment(lib,"Ws2_32.lib")

// http Ĭ�϶˿���80�����80�˿ڱ�ռ����ô�ĸ��˿ڼ���
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

    // ����տ��ܳ��ֵ�����������롢��̨�������뻹������ͬ
    // ԭ�򣺲���ջ�������� '\0'�ַ�Ϊֹ������ǰ��Ĳ���'\0' Ҳ��һ�����
    memset(recv_buf,0,sizeof(recv_buf));// ������0��ÿ�β���ǰ��Ҫ�ǵ��建�棬����ϰ�ߣ�
    if (recv(sAccept,recv_buf,sizeof(recv_buf),0) == SOCKET_ERROR)   //���մ���
    {
        printf("recv() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }       
    else
        printf("recv data from client:%s\n",recv_buf); //���ճɹ�����ӡ������

    //�����������
    i = 0; j = 0;
    // ȡ����һ�����ʣ�һ��ΪHEAD��GET��POST
    while (!(' ' == recv_buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = recv_buf[j];
        i++; j++;
    }
    method[i] = '\0';   // ������������Ҳ�ǳ�ѧ�ߺ����׺��ӵĵط�

    // �������GET��HEAD��������ֱ�ӶϿ���������
    // ��������Ĺ淶Щ���Է��������һ��501δʵ�ֵı�ͷ��ҳ��
    if (stricmp(method, "GET") && stricmp(method, "HEAD"))
    {
        closesocket(sAccept); //�ͷ������׽��֣�������ÿͻ���ͨ��
        printf("not get or head method.\nclose ok.\n");
        printf("***********************\n\n\n\n");
        return USER_ERROR;
    }
    printf("method: %s\n", method);

    // ��ȡ���ڶ�������(url�ļ�·�����ո����)������'/'��Ϊwindows�µ�·���ָ���'\'
    // ����ֻ���Ǿ�̬����(����url�г���'?'��ʾ�Ǿ�̬����Ҫ����CGI�ű���'?'������ַ�����ʾ���������������'+'����
    // ���磺www.csr.com/cgi_bin/cgi?arg1+arg2 �÷�����ʱҲ�в�ѯ�����ڳ���������)
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

    // �������url·��ת��Ϊ����·��
    _getcwd(path,_MAX_PATH);
    strcat(path,url);
    printf("path: %s\n",path);

    // �򿪱���·���µ��ļ������紫������r�ı���ʽ�򿪻����
    FILE *resource = fopen(path,"rb+");///*******************///

    // û�и��ļ�����һ���򵥵�404-file not found��htmlҳ�棬���Ͽ���������
    if(resource==NULL)
    {
        file_not_found(sAccept);
        // ���method��GET�������Զ����file not foundҳ��
        if(0 == stricmp(method, "GET"))
            send_not_found(sAccept);

        closesocket(sAccept); //�ͷ������׽��֣�������ÿͻ���ͨ��
        printf("file not found.\nclose ok.\n");
        printf("***********************\n\n\n\n");
        return USER_ERROR;
    }

    // ����ļ����ȣ��ǵ������ļ�ָ�뵽�ļ�ͷ
    fseek(resource,0,SEEK_SET);//�����ļ�ָ��stream��λ�á�
							//���ִ�гɹ���stream��ָ����SEEK_SETΪ��׼��ƫ��0��ָ��ƫ���������ֽڵ�λ�ã���������0��
							//���ִ��ʧ��(����offset�����ļ������С)���򲻸ı�streamָ���λ�ã���������һ����0ֵ��
    fseek(resource,0,SEEK_END);
    long flen=ftell(resource);//�õ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ���
    printf("file length: %ld\n", flen);
    fseek(resource,0,SEEK_SET);

	char* extension = file_type(url);
	file_ok(sAccept,flen,extension);   // ����200 OK HEAD

    // �����GET���������������Դ
    if(0 == stricmp(method, "GET"))
    {
        if(0 == send_file(sAccept, resource))
            printf("file send ok.\n");
        else
            printf("file send fail.\n");
    }
    fclose(resource);

    closesocket(sAccept); //�ͷ������׽��֣�������ÿͻ���ͨ��
    printf("close ok.\n");
    printf("***********************\n\n\n\n");

    return 0;

}

// ����404 file_not_found��ͷ
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

//�ж��ļ����ͣ����ļ���׺����.html,.jpg,.css�ȣ�
char* file_type(char* arg) {
	char * temp;         
	if ((temp = strrchr(arg, '.')) != NULL) {   //�����ַ���ָ���ַ����д����濪ʼ�����һ�γ��ֵ�λ�� 
		return temp + 1;
	}
	return "";          
}

// ����200 ok��ͷ
int file_ok(SOCKET sAccept, long flen, char* extension)
{
    char send_buf[MIN_BUF]; 
//  time_t timep;
//  time(&timep);
    sprintf(send_buf, "HTTP/1.1 200 OK\r\n");//�Ѹ�ʽ��������д��ĳ���ַ�����������
    send(sAccept, send_buf, strlen(send_buf), 0);//������һ���Ѿ����ӵ�socket�������ݣ�
												 //����޴��󣬷���ֵΪ���������ݵ����������򷵻�SOCKET_ERROR��
    sprintf(send_buf, "Connection: keep-alive\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
//  sprintf(send_buf, "Date: %s\r\n", ctime(&timep));
//  send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, SERVER);//���ñ�ͷServer
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

    sprintf(send_buf, "Content-Length: %ld\r\n", flen);
	printf("%s\n",send_buf);
    send(sAccept, send_buf, strlen(send_buf), 0);

	char* content_type = "Content-Type: text/plain\r\n";

	if (strcmp(extension, "html") == 0) {    //��������Ϊhtml
		content_type = "Content-Type: text/html\r\n";
	}
	if (strcmp(extension, "htm") == 0) {    //��������Ϊhtml
		content_type = "Content-Type: text/htm\r\n";
	}
	if (strcmp(extension, "ico") == 0) {    //��������Ϊico
		content_type = "Content-Type: image/ico\r\n";
	}
	if (strcmp(extension, "css") == 0) {    //��������Ϊcss
		content_type = "Content-Type: text/css\r\n";
	}
	if (strcmp(extension, "gif") == 0) {    //��������Ϊgif
		content_type = "Content-Type: image/gif\r\n";
	}
	if (strcmp(extension, "jpg") == 0) {    //��������Ϊjpg
		content_type = "Content-Type: image/jpg\r\n";
	}
	if (strcmp(extension, "png") == 0) {    //��������Ϊpng
		content_type = "Content-Type: image/png\r\n";
	}
	if (strcmp(extension, "js") == 0) {    //��������Ϊjavascript
		content_type = "Content-Type: application/x-javascript\r\n";
	}
	/*if (strcmp(extension, "txt") == 0) {    //��������Ϊtxt
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

// �����Զ����file_not_foundҳ��
int send_not_found(SOCKET sAccept)
{
    char send_buf[MIN_BUF];
    sprintf(send_buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "<BODY><h1 align='center'>404</h1><br/><h1 align='center'>�Ҳ���ҳ��.</h1>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    sprintf(send_buf, "</BODY></HTML>\r\n");
    send(sAccept, send_buf, strlen(send_buf), 0);
    return 0;
}

// �����������Դ
int send_file(SOCKET sAccept, FILE *resource)
{
    char send_buf[409600];
    while (1)
    {
        fread(send_buf,40950,1,resource);     //��һ���ļ����ж�����,��һ���ļ����ж����ݣ�
											  //����ȡcount���ÿ����size���ֽ�
        if (SOCKET_ERROR == send(sAccept, send_buf, 40960, 0))
        {
            printf("send() Failed:%d\n",WSAGetLastError());
            return USER_ERROR;
        }
        if(feof(resource))//������ϵ��ļ�������������ļ��������򷵻ط�0ֵ
            return 0;
    }   
}

int main()
{
    WSADATA wsaData;				//�����洢��WSAStartup�������ú󷵻ص�Windows Sockets����
    SOCKET sListen,sAccept;        //�����������׽��֣������׽���
    int serverport=DEFAULT_PORT;   //�������˿ں�
    struct sockaddr_in ser,cli;   //��������ַ���ͻ��˵�ַ
    int iLen;					  //�õ���ַ�ṹ��ĳ���  accept������Ҫ

    printf("--------------------------------\n");
    printf("server is ready in listening ...\n");
    printf("--------------------------------\n");

    //��һ��������Э��ջ
    if (WSAStartup(MAKEWORD(2,2),&wsaData) !=0)//�жϱ����汾�Ƿ����
    {
        printf("Failed to load Winsock.\n");
        return USER_ERROR;
    }

    //�ڶ��������������׽��֣����ڼ����ͻ�����
    sListen =socket(AF_INET,SOCK_STREAM,0);
    if (sListen == INVALID_SOCKET)//����׽��ִ���ʧ���򷵻�INVALID_SOCKET
    {
        printf("socket() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //������������ַ��IP+�˿ں�
    ser.sin_family=AF_INET;					 //����һ������,,sin_family��ʾЭ���,һ����AF_INET��ʾTCP/IPЭ��.
    ser.sin_port=htons(serverport);          //�������˿ں�
    ser.sin_addr.s_addr=htonl(INADDR_ANY);   //������IP��ַ��Ĭ��ʹ�ñ���IP
											 //in_addr��һ��������,��������Ϳ���ʹ�ö��ַ�ʽ��ʾIP��ַ

    //���������󶨼����׽��ֺͷ�������ַ
    if (bind(sListen,(LPSOCKADDR)&ser,sizeof(ser))==SOCKET_ERROR)
    {
        printf("blind() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }

    //���岽��ͨ�������׽��ֽ��м���
    if (listen(sListen,5)==SOCKET_ERROR)
    {
        printf("listen() Failed:%d\n",WSAGetLastError());
        return USER_ERROR;
    }
    while (1)  //ѭ���ȴ��ͻ�������
    {
        //�����������ܿͻ��˵��������󣬷�����ÿͻ������������׽���
        iLen=sizeof(cli);//�õ���ַ�ṹ��ĳ���  accept������Ҫ
        sAccept=accept(sListen,(struct sockaddr*)&cli,&iLen);////accept���� ֪�����µ�����
        if (sAccept==INVALID_SOCKET)
        {
            printf("accept() Failed:%d\n",WSAGetLastError());
            break;
        }
        //���߲��������߳̽������������
        DWORD ThreadID;
        CreateThread(NULL,0,SimpleHTTPServer,(LPVOID)sAccept,0,&ThreadID);  
    }
    closesocket(sListen);
    WSACleanup();
    return 0;
}