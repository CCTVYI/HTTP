#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX 1024
#define HOME_PAGE "index.html"
#define NOTFIND_PAGE "404.html"

//HTTP(超文本传输协议) 多线程 epoll C/S模型 支持中小型应用 cgi技术 shell脚本 mysql_C 3306
//world wide web
//HTTP/1.1主流 TCP IP DNS 
//HTTP无连接 无状态（session cookie）
//HTTP请求与响应

//创建套接字
int startup(int port)
{
	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=(INADDR_ANY);
	addr.sin_port=htons(port);
	int listen_sock=socket(AF_INET,SOCK_STREAM,0);
	if(listen_sock<0)
	{
		perror("socket");
		exit(2);  //终止服务
	}

	//端口复用，防止服务器重启时，之前绑定的端口还未释放或程序突然退出系统没有释放端口，那么下次绑定同一端口号就会出错。对于TCP连	  //接，进行四次挥手时，我们还要处于TIME_WAIT状态，使用了端口复用后，告诉系统只要一个端口处于TIME_WAIT状态，我们就不需要进行等待    //，直接对端口号进行使用。此状态是为了让网络中残余的TCP包消失,所以必须存在。
	int opt=1;
	setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	int ret=bind(listen_sock,(struct sockaddr*)&addr,sizeof(addr));
	if(ret<0)
	{
		perror("bind");
		exit(3);
	}
	ret=listen(listen_sock,10);
	if(ret<0)
	{
		perror("listen");
	}
	return listen_sock;
}

int get_line(int sock,char line[],int num)
{
	char c='a';
	int i=0;

	//读到'\n'，就不需要转换了
	while(i<num-1&&c!='\n')
	{
		//从sock中读一个长度，读到c缓冲区中。
		ssize_t s=recv(sock,&c,1,0);
		if(s>0)
		{
			if(c=='\r')
			{
				//窥探一次,数据只是被复制到了缓冲区，并不从输入队列中删除。
				recv(sock,&c,1,MSG_PEEK);  
				//如果是'\n'，就将其读走
				if(c=='\n')
				{
					recv(sock,&c,1,0);
				}
				else
				{
					c='\n';	
				}
			}
			line[i++]=c;
		}
		else
		{
			break;  //没有读到直接返回
		}
	}

	//使字符串以'\0'结尾,方便输出
	//空行时i=1，是\n\0
	line[i]='\0';
	return i;
}

//虽然此时是GET方法，并且没有参数，没有正文，也需要清理报文，消除sock的数据。
void clear_header(int sock)
{
	char line[MAX];
	do
	{
		get_line(sock,line,MAX);
	}while(strcmp(line,"\n"));
}

//响应
int echo_www(int sock,char *path,int size)
{
	clear_header(sock);

	//响应
	int fd=open(path,O_RDONLY);
	if(fd<0)
	{
		return 404;
	}
	char line[MAX];

	//格式化的数据写入缓冲区
	//******************响应报文必须按HTTP协议响应，报头信息必须有换行符，以行为单位**************************
	sprintf(line,"HTTP/1.0 200 OK\r\n");
	send(sock,line,strlen(line),0);
	sprintf(line,"Content-Type:text/html;charset=ISO-8895-1\r\n");  
	send(sock,line,strlen(line),0);

	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);

	//sendfile，在内核中复制，正文
	sendfile(sock,fd,NULL,size);
	return 200;
}

void echo_error(int code)
{
	switch(code){
		case 404:

			break;
		case 501:
			break;
		case 503:
			break;
		default:
			break;
	}
}

int exe_cgi(int sock,char path[],char method[],char *query_string)
{
	char line[MAX];
	char method_env[MAX];
	char query_string_env[MAX];
	char content_length_env[MAX/8];
	int content_length=-1;
	if(strcasecmp(method,"GET")==0)
	{
		clear_header(sock);	
	}
	//参数在正文里，正文不能多读，会产生粘包问题
	else
	{
		do
		{
			get_line(sock,line,MAX);      //原来写的sizeof(MAX) 。。。。。
			printf("--%s",line);
			if(strncmp(line,"Content-Length: ",16)==0)
			{
				content_length=atoi(line+16);	//报文的内容都是字符串			
			}
		}while(strcmp(line,"\n"));  //将报头全读完了
		if(content_length==-1)
		{
			return 404;
		}
	}
	
	//站在子进程的角度
	int input[2];
	int output[2];

	pipe(input);
	pipe(output);

	//单进程的WEB服务器，不能进行替换，需要创建子进程，替换代码段和数据段
	pid_t id=fork();
	if(id<0)
	{
		return 404;
	}
	else if(id==0)
	{
		//第一个参数：哪个路径下的哪个程序
		//第二个参数，想怎么执行
		//method,GET[query_string],POST[content-length]
		//利用环境变量，父进程的环境变量被子进程继承，环境变量具有全局特性。
		//子进程需要将结果告诉父进程，互相通信

		close(input[1]);  //子进程读，将写关闭
		close(output[0]);  //子进程写，将读关闭
		
		//必须知道管道的文件描述符是几
		//通过重定向，从标准输入、输出重定向到管道描述符中
		//int dup2(int oldfd, int newfd);  new是old的拷贝，new被old覆盖了。相当于0号文件描述符被3号管道覆盖了。
		
		dup2(input[0],0);
		dup2(output[1],1);

		sprintf(method_env,"METHOD=%s",method);
		putenv(method_env);
		if(strcasecmp(method,"GET")==0)
		{
			sprintf(query_string_env,"QUERY_STRING=%s",query_string);
			putenv(query_string_env);
		}
		else
		{//POST
			
			sprintf(content_length_env,"CONTENT_LENGTH=%d",content_length);
			putenv(content_length_env);
		}
		execl(path,path,NULL);
		exit(1);
	}
	else
	{
		close(input[0]);
		close(output[1]);

		char c;
		if(strcasecmp(method,"POST")==0)
		{
			int i=0;
			for(;i<content_length;i++)
			{
				recv(sock,&c,1,0);
				write(input[1],&c,1);
			}
		}
		
		
		sprintf(line,"HTTP/1.0 200 OK\r\n");
		send(sock,line,strlen(line),0);
		sprintf(line,"Content-Type:text/html;charset=ISO-8895-1\r\n");  
		send(sock,line,strlen(line),0);
		sprintf(line,"\r\n");
		send(sock,line,strlen(line),0);

		//子进程的运行结果
		while(read(output[0],&c,1)>0)
		{
			send(sock,&c,1,0);
		}

		waitpid(id,NULL,0);  //线程在阻塞式的等，是进程的一个分支

		close(input[1]);
		close(output[0]);
	}


}

void *handler_request(void *arg)
{
	int sock=(int)arg;
	//按行获取 \r  \n  \r\n 统一转换为\n
	char line[MAX];
	char method[MAX/32];
	char url[MAX];
	int errCode=200;
	char path[MAX];
	int cgi=0;  //传参了以cgi模式运行：GET方法中url里面有参数，POST方法
	char *query_string=NULL;

	//条件编译:满足某条件时对一组语句进行编译，而当条件不满足时则编译另一组语句。
#ifdef Debug
	do
	{
		//将缓冲区带进去
		get_line(sock,line,MAX);
		printf("%s",line);  //line中有'\0'，输出一行后，缓冲区被刷新。
	}while(strcmp(line,"\n"));  //读到空行就不读了。
#else
	get_line(sock,line,sizeof(line));  //读一行
	int i=0;
	int j=0;

	//获取方法,不能是空格，条件都成立
	while(i<sizeof(method)-1 && j<sizeof(line) && !isspace(line[j]))
	{
		method[i]=line[j];
		i++;j++;
	}
	method[i]='\0';

	if(strcasecmp(method,"GET")==0)  //strcasecmp不区分大小写
	{}
	else if(strcasecmp(method,"POST")==0)
	{
		cgi=1;
	}
	else
	{
		errCode=404;
		goto end;
	}
	
	//空格时跳过
	while(j<sizeof(line) && isspace(line[j]))
	{
		j++;
	}

	//	获取资源,取网址时，应该一个字符一个字符赋值，遇到空格停下来
	i=0;
	while(i<sizeof(url)-1 && j<sizeof(line) && !isspace(line[j]))
	{
		url[i]=line[j];
		i++;j++;
	}

	url[i]='\0';

	//method,cgi,url[/a/b/c/d.java?a=50&b=100]
	if(strcasecmp(method,"GET")==0)
	{
		query_string=url;

		//url指向网址，query_string指向参数,处理参数
		while(*query_string)
		{
			if(*query_string=='?')
			{
				*query_string='\0';
				query_string++;
				cgi=1;
				break;
			}
			query_string++;  //不然会死循环，while条件会一直成立
		}
	}

	//url
	//网址追加wwwroot/index.html
	sprintf(path,"wwwroot%s",url);
	//先处理根目录，wwwroot/
	if(path[strlen(path)-1]=='/')
	{
		strcat(path,HOME_PAGE);
	}

	printf("method:%s path:%s\n",method,path);


	//wwwroot/index.html
	//是否存在首页，通过调用stat，获取文件inode信息
	struct stat st;
	if(stat(path,&st)<0)
	{
		errCode=404;
		goto end;
	}
	else  //存在
	{
		//是一个目录
		if(S_ISDIR(st.st_mode))
		{
			strcat(path,HOME_PAGE);
		}
		//二进制文件，有可执行权限
		else if((st.st_mode & S_IXUSR) || \
			(st.st_mode & S_IXGRP) || \
			(st.st_mode) & S_IXOTH)
			{
				cgi=1;
			}
		else
		{

		}

		//让我们的HTTP去执行一个程序，这种方式成为CGI，是外部应用程序与web服务器之间的接口标准。可以使用CGI使网站可以进行交互
		//CGI是Web服务器运行时外部程序的规范,按CGI 编写的程序可以扩展服务器功能。CGI 应用程序能与浏览器进行交互,还可通过数据
		//库API与数据库服务器等外部数据源进行通信,从数据库服务器中获取数据。格式化为HTML文档后，发送给浏览器，也可以将从浏览
		//器获得的数据放到数据库中。几乎所有服务器都支持CGI,可用任何语言编写CGI,包括流行的C、C ++、VB 和Delphi 等。
		if(cgi)
		{
			exe_cgi(sock,path,method,query_string);
		}
		else
		{
			errCode=echo_www(sock,path,st.st_size);
		}

	}
	//是否存在首页，通过调用stat，获得文件的属性
#endif
end:
	if(errCode!=200)
	{
		echo_error(errCode);
	}
	close(sock);
}

int main(int argc,char* argv[])
{
	if(argc!=2)
	{
		printf("Usage:[./httpd] [port]\n");
		return 1;
	}
    int listen_sock=startup(atoi(argv[1]));

	//忽略SIG
	signal(SIGPIPE,SIG_IGN);

	for(;;)
	{
		//获得的新连接放入结构体client中
		struct sockaddr_in client;
		socklen_t len=sizeof(client);
		int new_sock=accept(listen_sock,(struct sockaddr*)&client,&len);
		if(new_sock<0)
		{
			perror("accept");
			continue;
		}

		//创建多线程
		pthread_t id;
		pthread_create(&id,NULL,handler_request,(void*)new_sock);
		pthread_detach(id);  //新线程去处理请求，将其进行分离，系统自动回收，主线程继续接受连接请求。

	}
	return 0;
}
