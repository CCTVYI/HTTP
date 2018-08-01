#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//34班

void cal(char *data)
{
	//first_data=100&second_data=200
	//此处参数在正文开头
	int a,b;
	printf("method:%s",getenv("METHOD"));
	printf("-----%s\n",data);
	sscanf(data,"first_data=%d&second_data=%d",&a,&b);  //将数据输入到某个变量中
	
	printf("<html>\n");

	printf("<h3>%d + %d = %d</h3>\n",a,b,a+b);
	printf("<h3>%d - %d = %d</h3>\n",a,b,a-b);
	printf("<h3>%d * %d = %d</h3>\n",a,b,a*b);
	printf("<h3>%d / %d = %d</h3>\n",a,b,a/b);
	printf("<h3>%d %% %d = %d<h3>\n",a,b,a%b);
	printf("</html>\n");
}

int main()
{
	char method[32];
	char buf[1024];
	if(getenv("METHOD")!=NULL)
	{
		strcpy(method,getenv("METHOD"));
		if(strcasecmp(method,"GET")==0)
		{
			strcpy(buf,getenv("QUERY_STRING"));
		}
		else if(strcasecmp(method,"POST")==0)
		{
			int content_len=atoi(getenv("CONTENT_LENGTH"));
			char c;
			int i=0;
			//父进程已经把数据写好了
			for(;i<content_len;i++)
			{
				read(0,&c,1);
				buf[i]=c;
			}
			buf[i]='\0';
		}
		else
		{}
		
		cal(buf);
	}
	return 0;
}
