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

//1.���ܴ�������
void usage(const char* argv)
{ 
  printf("%s:[ip][port]\n",argv);
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
