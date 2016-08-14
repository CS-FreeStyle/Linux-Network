#include<stdio.h>
#include<stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include<sys/stat.h>
#include<unistd.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define SIZE 1024

#define _LINING_
#define _ZHANG_

//1.���ܴ�������
void usage(const char* argv)
{ 
  printf("%s:[ip][port]\n",argv);
}

void echo_errno(int sock)
{
    printf("he he, you are fool...\n");

}

void clear_header(int sock)
{
    char buf[SIZE];
    char len = SIZE;
    char ret = -1;
    do
    {
	ret = get_line(sock,buf,len);
    }while((ret > 0) && strcmp(buf,"\n") != 0);
}

static void exe_cgi(int sock,const char* method,const char* path,const char* query_string)
{
    char buf[SIZE];
    int content_length = -1;
    int ret = -1;
    int cgi_input[2];
    int cgi_output[2]; //�����ܵ�
    char method_env[SIZE];
    char query_string_env[SIZE];
    char content_length_env[SIZE];
///////////////////////////////////////////////////////
    printf("method %s\n",method);
    printf("path %s\n",path);
    printf("data:%s",query_string);
///////////////////////////////////////////////////////
    
    if(strcasecmp(method,"GET") == 0)
    {
	clear_header(sock);
    }
    else  //POST���
    {
	do
	{
	    ret = get_line(sock,buf,sizeof(buf));
	    if(strncasecmp(buf,"Content-Length: ",16) == 0)
	    {
		content_length = atoi(&buf[16]);
	    }
	}while((ret > 0) && (strcmp(buf,"\n") != 0));

	if(content_length == -1)
	{
	    echo_errno(sock);
	    return;
	}
    }

    sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n");
    send(sock,buf,strlen(buf),0);  //��֪ͨ���ͻ���

    if(pipe(cgi_input) < 0)
    {
	echo_errno(sock);
	return;
    }

    if(pipe(cgi_output) < 0)
    {
	echo_errno(sock);
	return;
    }

    pid_t id = fork();   //�����ӽ���
    if(id == 0)
    {
	close(cgi_input[1]);
	close(cgi_output[0]);

	dup2(cgi_input[0],0);
	dup2(cgi_output[1],1);

	sprintf(method_env,"REOUEST_METHOD=%s",method);
	putenv(method_env);

	if(strcasecmp(method,"GET") == 0)
	{
	    sprintf(query_string_env,"QUERY_STRING=%s",query_string);
	    putenv(query_string_env);
	}
	else
	{
	    sprintf(content_length_env,"CONTENT_LENGTH=%s",content_length);
	    putenv(content_length_env);
	}

	execl(path,path,NULL);
	exit(1);
    }
    else   //��
    {
	close(cgi_input[0]);
	close(cgi_output[1]);

	char c = '\0';
	int i = 0;

	if(strcasecmp(method,"POST") == 0)
	{
	    for(; i < content_length; i++)
	    {
		recv(sock,&c,1,0);
		printf("%c",c);
		write(cgi_input[1],&c,1);
	    }
	}

	printf("\n");

	int ret = 0;
	while((ret = read(cgi_output[0],&c,1)) > 0)
	{
	    send(sock,&c,1,0);
	}

	waitpid(id,NULL,0);
    }
}

static void echo_www(int sock,const char* path,ssize_t size)
{
    int fd = open(path,O_RDONLY);
    if(fd < 0)
    {
	echo_errno(sock);
	return;
    }

    printf("get a new client %d --> %s...\n",sock,path);

    char status_line[SIZE];
    sprintf(status_line,"HTTP/1.0 200 OK\r\n\r\n");
    send(sock,status_line,strlen(status_line),0);   //write

    if(sendfile(sock,fd,NULL,size) < 0)
    {
	echo_errno(sock);
	return;
    }

    close(fd);
}

int get_line(int sock,char buf[],int buflen)
{
    if(!buf || buflen < 0)
    {
	return -1;
    }

    int i = 0;
    char c = '\0';
    int ret = 0;
    //�������������н����������
    while((i < buflen-1) && c != '\n')
    {
	ret = recv(sock,&c,1,0);   //�൱��read��������һ���ַ��ŵ�c�� send �൱��write �ɹ����ض�ȡ������
	if(ret > 0)
	{
	    if(c == '\r')
	    {
		//�ڶ���һ���ַ���ֻ���뿴��
		ret = recv(sock,&c,1,MSG_PEEK);  //��̽��һ���ַ��������ӻ��������ɾ��
		if(ret > 0 && c == '\n')
		{
		    recv(sock,&c,1,0); //��ſ�ʼ���ڶ����ַ�
		}
		else  //\r
		{
		    c = '\n';
		}
	    }
	    buf[i++] = c;
	}
	else
	{
            c = '\n';
	}
    }	
    buf[i] = '\0';
    return i;
}

static void *handler_data(void* arg)
{
	int sock = int(arg);
	char buf[SIZE];
	int len = sizeof(buf)/sizeof(buf[0]);
	
	char method[SIZE/15];
	char url[SIZE];
	char path[SIZE];
	
	int i, j;
	int cgi = 0;
	char *query_string = NULL;
	int ret = -1;
	
#define _LINING_      //�ȴ�ӡһ�� ��Ч��
	do
	{
		ret = get_line(sock,buf,len);   //��ȡһ��
		printf("%s",buf);
		fflush(stdout);                 //�ϱ�û��\nǿ��ˢ����
	}
	while((ret > 0) && (strcmp(buf,'\n')) != 0);  //������Ч���� ���ӡһ�� ���һ������
#endif
	ret = get_line(sock,buf,len);
	printf("%d\n",ret);
	if(ret <= 0)
	{
	    echo_errno(sock);
	    return (void*)1;
	}

	i = 0;  //method �������±�
	j = 0;  //buf�Լ�
	while(i < (sizeof(method)-1) && (j < sizeof(buf)) && (!isspace(buf[j])))
	{
	    method[i] = buf[j];  //��ȡmethod
	    i++;
	    j++;
	}

	method[i] = '\0';
//#ifdef _LINING_
	printf("method  %s  \n",method);
//#endif
	//ͷ���ķ���������  ��ȫ���Խ�����  ��ȡ�Ĳ���
	if(strcasecmp(method,"GET") != 0 && strcasecmp(method,"POST") != 0)
	{
	    echo_errno(sock);
	    return (void*)2;
	}

	//url
	i = 0;
	if(isspace(buf[j]))  //���������ڿո񷵻ط���ֵ
	{
	    j++;
	}

	while((i < sizeof(url)-1) && (j < sizeof(buf)) && (!isspace(buf[j])))
	{
	    url[i] = buf[j];
	    i++;
	    j++;
	}

	url[i] = '\0';
//#ifdef _LINING_
	printf("url  %s  \n",url);
//#endif

	//���ڿ������ַ���
	if(strcasecmp(method,"POST") == 0)
	{
	    cgi = 1;
	}

	if(strcasecmp(method,"GET") == 0)
	{
	    query_string = url;  //
	    while(*query_string != '\0' && *query_string != '?')
	    {
		query_string++;
	    }

	    printf("query_string  %c\n",*query_string);
	    //�����?�� �ʹ����в���
	    if(*query_string == '?')
	    {
		cgi = 1;
		*query_string = '\0'; //��?���滻��
		query_string++;       //����ָ������ĵ�һ��λ�ã�
		printf("query_string %s\n",query_string);
	    }
	}

	sprintf(path,"htdoc%s",url);   //url����htdocǰ׺ ��Ϊ���յ�·��
	if(path[strlen(path)-1] == '/') //������һ���ַ���/ �Ǿʹ�����ʱ��ط������µ����Ŀ¼�µĶ���
	{
	    strcat(path,"index.html");  //һ������·��
	}

	struct stat st;   //Linux���ļ��Ĳ���
	if(stat(path,&st) < 0)   //��ȡ�ļ�����  �ɹ���ȡ������
	{
	    printf("%s\n",path);
	    echo_errno(sock);
	    return (void*)3;
	}
	else  //�ɹ���
	{
	    if(S_ISDIR(st.st_mode))  //�жϴ��ļ��ǲ���Ŀ¼
	    {
		strcat(path,"index.html");
	    }
	    else if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
	    {
		cgi = 1;     //�ж��ļ����� Ϊ �û� �� ���� ��ִ��  ǰ��������һ���ļ���
	    }
	    else
	    {
		printf("he he here is wrong!\n");
	    }
	    
	    if(cgi)
	    {
		printf("here is cgi...\n");
		exe_cgi(sock,method,path,query_string); //�в��� ����POST����
	    }
	    else
	    {
		//printf("here is www...\n");;
		clear_header(sock);  //�������
		echo_www(sock,path,st.st_size);   //������ͨ�ļ�
	    }
	}

	close(sock);
	return (void*)0;
}

//����һ���׽��֣��󶨣���������
static int start_up(char* ip,int port)  
{
    //sock
    //1.�����׽���
    int sock=socket(AF_INET,SOCK_STREAM,0);   
    if(sock<0)
    {
        perror("sock");
        exit(2);
    }
  
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
  
    //2.��䱾�� sockaddr_in �ṹ�壨���ñ��ص�IP��ַ�Ͷ˿ڣ�
    struct sockaddr_in local;       
    local.sin_port=htons(port);
    local.sin_family=AF_INET;
    local.sin_addr.s_addr=inet_addr(ip);

    //3.bind������
    if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0) 
    {
        perror("bind");
        exit(3);
    }
    //4.listen�������� ��������
    if(listen(sock,5)<0)
    {
        perror("sock");
        exit(4);
    }
  return sock;    //�������׽��ַ���
}

//������
int main(int argc,char *argv[])
{
    if(argc != 3)
	{
		usage(argv[0]);
		exit(1);
	}
	
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);

	int listen_sock = start_up(argv[1],atoi(argv[2]));
	int done = 0;

	while(!done)
    {
		//���տͻ�������������
		int new_fd = accept(listen_sock,(struct sockaddr*)&peer,&len);

		if(new_fd > 0)
		{
			pthread_t id;
			//��һ�����߳�... 
			pthread_create(&id,NULL,handler_data,(void*)new_fd);
			pthread_detach(id);    //ÿ����һ���̣߳�������룬��������
		}
    }
    return 0;
}
