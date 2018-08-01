#include <stdio.h>
#include <mysql/mysql.h>

void insert_data(char *name,char *sex,int *tel)
{
	printf("%s\n",mysql_get_client_info());

	//申请数据库句柄，对数据库进行初始化
	MYSQL *mysql_fd=mysql_init(NULL);
	//端口号可以进行配置，公司有可能会使用自己私有的端口号 
	if(mysql_real_connect(mysql_fd,"127.0.0.1","root","520803","guo",0,NULL,0) == NULL)   
	{
		printf("connect failed!\n");	
	}
	printf("mysql success\n");

	//第一种插入：通过在命令行中直接输入命令。
	//const char *sql="INSERT INTO student_info (name,sex,tel) VALUES(\"durong\",\"man\",\"12345\")";   //命令里本来就需要双引号
	
	//第二种插入：通过网络插入数据。
	char sql[1024];
	sprintf(sql,"INSERT INTO student_info(name,sex,tel) VALUES(\"guozhichao\",\"man\",\"678910\")");
	printf("%s\n",sql);
	mysql_query(mysql_fd,sql);

	mysql_close(mysql_fd);
}

int main()
{
	char *name;
	char *sex;
	int *tel;
	char query_string[1024];
	if(getenv("METHOD")==get)
	{
		strcpy(query_string,getenv("QUERY_STRING"));			
	}
	else
	{
		query_string=atoi(getenv("CONTENT_LENGTH"));
	}

	insert_data(name,sex,tel);
	return 0;
}
