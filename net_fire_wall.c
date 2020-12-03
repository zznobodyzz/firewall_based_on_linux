#include<time.h>
#include<sys/time.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sqlite3.h>
#include<string.h>
#include<signal.h>
#include<semaphore.h>
#include<dirent.h>
#include<sys/ioctl.h>
#define MAGIC_NUM 'Z'
#define ADD_IP _IOW(MAGIC_NUM,0,int)
#define ADD_SRC_PORT _IOW(MAGIC_NUM,1,int)
#define ADD_DST_PORT _IOW(MAGIC_NUM,2,int)
#define DEL_IP _IOW(MAGIC_NUM,3,int)
#define DEL_SRC_PORT _IOW(MAGIC_NUM,4,int)
#define DEL_DST_PORT _IOW(MAGIC_NUM,5,int)
#define MAX_CMD 128
#define MAX_IP 16
sqlite3 * db;
void main_menu(void)
{
	system("clear");
	printf("\n\t\t***************************************************************************\n");
	printf("\n\t\t*                            net fire wall                                *\n");
	printf("\n\t\t*                           1.          ip                                *\n");
	printf("\n\t\t*                           2.         port                               *\n");
	printf("\n\t\t*                           0.         exit                               *\n");
	printf("\n\t\t***************************************************************************\n\t\t");
}
void ip_menu(void)
{
	system("clear");
	printf("\n\t\t***************************************************************************\n");
	printf("\n\t\t*                           1.        add  ip                             *\n");
	printf("\n\t\t*                           2.        del  ip                             *\n");
	printf("\n\t\t*                           3.        print  ip                           *\n");
	printf("\n\t\t*                           0.        back                                *\n");
	printf("\n\t\t***************************************************************************\n\t\t");
}
void port_menu(void)
{
	system("clear");
	printf("\n\t\t***************************************************************************\n");
	printf("\n\t\t*                           1.     add source port                        *\n");
	printf("\n\t\t*                           2.     del source port                        *\n");
	printf("\n\t\t*                           3.     print source port                      *\n");
	printf("\n\t\t*                           4.     add dest port                          *\n");
	printf("\n\t\t*                           5.     del dest port                          *\n");
	printf("\n\t\t*                           6.     print dest port                        *\n");
	printf("\n\t\t*                           0.        back                                *\n");
	printf("\n\t\t***************************************************************************\n\t\t");
}
void print(char * table,char * elum)
{
	char cmd[MAX_CMD];
	char ** result;
	char * errmsg;
	int nrow;
	int ncolumn;
	int index;
	int i,j;
	sprintf(cmd,"select %s from %s",elum,table);
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
			printf("\t\tprint error\n");
			printf("\t\t%s\n",errmsg);
			return;
	}
	if(nrow!=0)
	{
		index=ncolumn;
		for(i=0;i<nrow;i++)
		{
				for(j=0;j<ncolumn;j++)
						printf("\t\t%s: %s\n",result[j],result[index++]);
				printf("\t\t---------------------\n");
		}
	}
	return;
}
unsigned long get_ipaddr(char * ipaddr)
{
	unsigned long data=0;
	int count=0;
	char * p=ipaddr;
	char buf[4]={0};
	int temp[4]={0};
	int i=0;
	int j=0;
	for(j=0;j<4;j++)
	{
		while(*p!='\0'&&*p!='.')
			buf[i++]=*p++;
		buf[i]='\0';
		temp[j]=atoi(buf);
		memset(buf,0,sizeof(buf));
		i=0;
		p++;
	}
	data=temp[0]<<24|temp[1]<<16|temp[2]<<8|temp[3];
	return htonl(data);
}
void my_ip_solution(char * ip,int choise)
{
	int fd;
	char ** result;
	char * errmsg;
	int nrow;
	int ncolumn;
	char ipaddr[MAX_IP];
	char cmd[MAX_CMD];
	unsigned long data=0;
	if(!ip)
	{
		printf("\t\tinput an ipaddress:\n\t\t");
		fgets(ipaddr,MAX_IP,stdin);
		ipaddr[strlen(ipaddr)-1]='\0';
		if(ipaddr[0]=='q')
		{
			printf("\t\tcanceled\n");
			return;
		}
	}
	else
		strcpy(ipaddr,ip);
	data=get_ipaddr(ipaddr);
	fd=open("/dev/net_fire_wall",O_RDWR);
	if(fd<0)
	{
		perror("\t\topen failed\n");
		exit(-1);
	}
	if(choise==0)
	{
		if(ioctl(fd,ADD_IP,data))
		{
			printf("\t\tadd error\n");
			return;
		}
	}
	else if(choise==1)
	{
		if(ioctl(fd,DEL_IP,data))
		{
			printf("\t\tdelete error\n");
			return;
		}
	}
	close(fd);
	sprintf(cmd,"select * from ip where ipaddr='%s'",ipaddr);
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
			printf("\t\tprint error\n");
			printf("\t\t%s\n",errmsg);
			return;
	}
	if(choise==0)
	{
		if(nrow==0)
		{
			sprintf(cmd,"insert into ip(ipaddr) values('%s')",ipaddr);
			if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
			{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
			}
		}
	}
	else if(choise==1)
	{
		if(nrow!=0)
		{
			sprintf(cmd,"delete from ip where ipaddr='%s'",ipaddr);
			if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
			{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
			}
		}
	}
	return;
}
void my_port_solution(char * port,int sig,int choise)
{
	int fd;
	char ** result;
	char * errmsg;
	int nrow;
	int ncolumn;
	char cmd[MAX_CMD];
	unsigned short data=0;
	if(!port)
	{
		if(sig==0)
			printf("\t\tinput an source port(local port)\n\t\t");
		else if(sig==1)
			printf("\t\tinput an destination port(out port)\n\t\t");
		scanf("%hu",&data);
		getchar();
	}
	else
		data=atoi(port);
	fd=open("/dev/net_fire_wall",O_RDWR);
	if(fd<0)
	{
		perror("\t\topen failed\n");
		exit(-1);
	}
	if(sig==0)
	{
		if(choise==0)
		{
			if(ioctl(fd,ADD_SRC_PORT,htons(data)))
			{
				printf("\t\tadd error\n");
				return;
			}
		}
		else if(choise==1)
		{
			if(ioctl(fd,DEL_SRC_PORT,htons(data)))
			{
				printf("\t\tdelete error\n");
				return;
			}
		}
	}
	else if(sig==1)
	{
		if(choise==0)
		{
			if(ioctl(fd,ADD_DST_PORT,htons(data)))
			{
				printf("\t\tadd error\n");
				return;
			}
		}
		else if(choise==1)
		{
			if(ioctl(fd,DEL_DST_PORT,htons(data)))
			{
				printf("\t\tdelete error\n");
				return;
			}
		}
	}
	close(fd);
	if(sig==0)
		sprintf(cmd,"select * from src_port where port=%d",(int)data);
	else if(sig==1)
		sprintf(cmd,"select * from dst_port where port=%d",(int)data);
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
			printf("\t\tprint error\n");
			printf("\t\t%s\n",errmsg);
			return;
	}
	if(choise==0)
	{
		if(nrow==0)
		{
			if(sig==0)
				sprintf(cmd,"insert into src_port(port) values(%d)",(int)data);
			else if(sig==1)
				sprintf(cmd,"insert into dst_port(port) values(%d)",(int)data);
			if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
			{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
			}
		}
	}
	else if(choise==1)
	{
		if(nrow!=0)
		{
			if(sig==0)
				sprintf(cmd,"delete from src_port where port=%d",(int)data);
			else if(sig==1)
				sprintf(cmd,"delete from dst_port where port=%d",(int)data);
			if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
			{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
			}
		}
	}
	return;
}
void print_ip(void)
{
	print("ip","ipaddr");
}
void print_port(int sig)
{
	if(sig==0)
		print("src_port","port");
	else if(sig==1)
		print("dst_port","port");
}
void del_all(void)
{
	char cmd[MAX_CMD];
	int i;
	char ** result;
	char * errmsg;
	int nrow;
	int ncolumn;
	int index;
	strcpy(cmd,"select ipaddr from ip");
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
		printf("\t\tdatabase error\n");
		printf("\t\t%s\n",errmsg);
		return;
	}
	if(nrow!=0)
	{		
		index=ncolumn;
		for(i=0;i<nrow;i++)
			my_ip_solution(result[index++],1);
	}
	strcpy(cmd,"select port from src_port");
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
		printf("\t\tdatabase error\n");
		printf("\t\t%s\n",errmsg);
		exit(-1);
	}
	if(nrow!=0)
	{		
		index=ncolumn;
		for(i=0;i<nrow;i++)
			my_port_solution(result[index++],0,1);
	}
	strcpy(cmd,"select port from dst_port");
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
		printf("\t\tdatabase error\n");
		printf("\t\t%s\n",errmsg);
		exit(-1);
	}
	if(nrow!=0)
	{		
		index=ncolumn;
		for(i=0;i<nrow;i++)
			my_port_solution(result[index++],1,1);
	}
}
void database_init(void)
{
	char cmd[MAX_CMD];
	int i;
	int ip_flag=0;
	int src_port_flag=0;
	int dst_port_flag=0;
	char ** result;
	char * errmsg;
	int nrow;
	int ncolumn;
	int index;
	if(sqlite3_open("./data",&db)!=SQLITE_OK)
	{				
		printf("\t\topen database failed\n");
		printf("\t\t%s\n",sqlite3_errmsg(db));
		exit(-1);
	}
	strcpy(cmd,"select name from sqlite_master where type='table'");
	if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
	{
		printf("\t\tsearch table name failed\n");
		printf("\t\t%s\n",errmsg);
		exit(-1);
	}
	if(nrow!=0)
	{
		for(i=1;i<=nrow;i++)
		{
			if(strcmp(result[i],"ip")==0)
				ip_flag=1;
			if(strcmp(result[i],"src_port")==0)
				src_port_flag=1;
			if(strcmp(result[i],"dst_port")==0)
				dst_port_flag=1;
		}
	}
	if(ip_flag==0)
	{
		strcpy(cmd,"create table ip(no integer primary key,ipaddr text)");
		if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
				exit(-1);
		}
	}
	else
	{
		strcpy(cmd,"select ipaddr from ip");
		if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
		{
			printf("\t\tdatabase error\n");
			printf("\t\t%s\n",errmsg);
			exit(-1);
		}
		if(nrow!=0)
		{		
			index=ncolumn;
			for(i=0;i<nrow;i++)
				my_ip_solution(result[index++],0);
		}
	}
	if(src_port_flag==0)
	{
		strcpy(cmd,"create table src_port(no integer primary key,port text)");
		if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
				exit(-1);
		}
	}
	else
	{
		strcpy(cmd,"select port from src_port");
		if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
		{
			printf("\t\tdatabase error\n");
			printf("\t\t%s\n",errmsg);
			exit(-1);
		}
		if(nrow!=0)
		{		
			index=ncolumn;
			for(i=0;i<nrow;i++)
				my_port_solution(result[index++],0,0);
		}
	}
	if(dst_port_flag==0)
	{
		strcpy(cmd,"create table dst_port(no integer primary key,port text)");
		if(sqlite3_exec(db,cmd,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
				printf("\t\tdatabase error\n");
				printf("\t\t%s\n",errmsg);
				exit(-1);
		}
	}
	else
	{
		strcpy(cmd,"select port from dst_port");
		if(sqlite3_get_table(db,cmd,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
		{
			printf("\t\tdatabase error\n");
			printf("\t\t%s\n",errmsg);
			exit(-1);
		}
		if(nrow!=0)
		{		
			index=ncolumn;
			for(i=0;i<nrow;i++)
				my_port_solution(result[index++],1,0);
		}
	}
}
void ip_operation(void)
{
	int choise;
	int fg=0;
	while(1)
	{
		ip_menu();
		scanf("%d",&choise);
		getchar();
		switch(choise)
		{
			case 1: my_ip_solution(NULL,0);
					break;
			case 2: my_ip_solution(NULL,1);
					break;
			case 3: print_ip();
					break;
			case 0: fg=1;
					break;
			default:printf("\t\tchoose again\n");
					break;
		}
		if(fg==1)
			return;
		printf("\t\tpress enter to continue\n");
		getchar();
	}
}
void port_operation(void)
{
	int choise;
	int fg=0;
	while(1)
	{
		port_menu();
		scanf("%d",&choise);
		getchar();
		switch(choise)
		{
			case 1: my_port_solution(NULL,0,0);
					break;
			case 2: my_port_solution(NULL,0,1);
					break;
			case 3: print_port(0);
					break;
			case 4: my_port_solution(NULL,1,0);
					break;
			case 5: my_port_solution(NULL,1,1);
					break;
			case 6: print_port(1);
					break;
			case 0: fg=1;
					break;
			default:printf("\t\tchoose again\n");
					break;
		}
		if(fg==1)
			return;
		printf("\t\tpress enter to continue\n");
		getchar();
	}
}
int main(void)
{
	int choise;
	int fg=0;
	database_init();
	while(1)
	{
		main_menu();
		scanf("%d",&choise);
		getchar();
		switch(choise)
		{
			case 1: ip_operation();
					break;
			case 2: port_operation();
					break;
			case 0: fg=1;
					break;
			default:printf("\t\tchoose again\n");
					break;
		}
		if(fg==1)
		{
			del_all();
			exit(0);
		}
		printf("\t\tpress enter to continue\n");
		getchar();
	}
}