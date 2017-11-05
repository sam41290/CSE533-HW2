/*-----------------------------------------------------------------------------------------------------------------//
	Creator: Soumyakant Priyadarshan									   //	
	SBU ID: 111580387											   //
	email: spriyadarsha@cs.stonybrook.edu									   //
														   //
   References:													   //
        1. https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c				   //
	2. http://www.geeksforgeeks.org/socket-programming-cc/							   //
	3. http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html						   //
	4. https://stackoverflow.com/questions/3437404/min-and-max-in-c                               		   //
	5. Addison Wesley : UNIX Network Programming Volume 1, Third Edition: The Sockets Networking API  	   //
-------------------------------------------------------------------------------------------------------------------*/



#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include<errno.h>
#include <sys/select.h>
#include <sys/file.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/fcntl.h>
#include<math.h>
#include<features.h>
#include<pthread.h>
#include"mytypes.h"
#include"unprtt.h"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ANSI_COLOR_BLUE    "\x1b[34m"

#define SERV_PORT 8080

static struct rtt_info rttinfo;
static int rttinit = 0;
FILE *fp;
char srv_ip[50];
int srv_port;
int seed;
float prob;
int bufsize;
int delay;
int freebufspace;
struct message *buffer;

int bufstart=0,bufend=0;
char *cmd;
char *redirection;

void *readbuf()
{

        while(1)
        {
		int ctr=0;
		while(ctr < bufsize)
		{
                	if(bufend==bufstart)
                        	continue;
                	if(bufstart!=bufend)
			{
				if(fp)
					fprintf(fp,"%s",buffer[bufstart].msg);
				else 
					printf("%s",buffer[bufstart].msg);
				bufstart=(bufstart + 1) % bufsize;
				ctr++;
			}	
			
		}
		sleep(1-((delay) * log(random())/1000));
        }
}





static void connect_alarm(int signo)
{
	//printf("alarm\n");
	return; 
}

int main(int argc,char *args[])
{

FILE *cli=fopen("./asgn2/client.in","r");
if(cli==NULL)
	printf("can not open client.in file");
fscanf(cli,"%s",srv_ip);
fscanf(cli,"%d",&srv_port);
fscanf(cli,"%d",&seed);
fscanf(cli,"%f",&prob);
fscanf(cli,"%d",&bufsize);
fscanf(cli,"%d",&delay);

fclose(cli);

cmd=(char *)malloc(sizeof(char) * 1000);
redirection=(char *)malloc(sizeof(char) * 1000);
buffer=malloc(sizeof(struct message)*(bufsize+1));
freebufspace = bufsize;
srand(seed);
printf("bufsize %d\n", bufsize);
printf("delay %d\n", delay);
srand(seed);
//printf("random %d\n", rand());
//printf("%f", 1-((delay) * log(random())/1000));

pthread_t pid;
pthread_create(&pid,NULL,&readbuf,NULL);

int clientfd=socket(AF_INET,SOCK_DGRAM,0);
if (clientfd < 0)
{
	printf("file client::%s:: socket() failed",args[2]);
	return 0;
}

struct sockaddr_in cliaddr;
struct sockaddr_in srvaddr;
memset(&srvaddr,0,sizeof(srvaddr));
srvaddr.sin_family=AF_INET;
srvaddr.sin_port=htons(SERV_PORT);

if(inet_pton(AF_INET, (args[1]), &srvaddr.sin_addr) < 0)
{
	printf("file client::%s:: wrong address format",args[2]);
}


while(1)
{
	int consts=connect(clientfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
	//printf("%d",errno);
	if (errno==ETIMEDOUT)
	{
        	printf("file client::%s:: Connection filed out",args[2]);
        	return 0;
	}
	if (errno==ECONNREFUSED)
	{
        	printf("file client::%s:: Connection refused by server",args[2]);
        	return 0;
	}
	if (errno==EHOSTUNREACH || errno==ENETUNREACH)
	{
        	printf("file client::%s:: Connection filed out",args[2]);
        	return 0;
	}
	freebufspace=abs(((bufsize + 1)-bufend)-((bufsize + 1)-bufstart) - 1);
	if(freebufspace<=0)
		continue;
	printf("\nenter command: ");
	char ch;
	ch=fgetc(stdin);
	int i=0;
	int red=0;
	int j=0;
	while (ch !=EOF && ch!='\n' && ch!='\0')
	{
		if(ch=='>')
			red++;
		if(red>=1)
		{
			if(ch!='>')
			{
				redirection[j]=ch;
				j++;
			}
		}
		else
		{
			cmd[i]=ch;
			i++;
		}
		ch=fgetc(stdin);
	}
	if(cmd[i-1]==' ')
		i=i-1;
	cmd[i]='\0';
	redirection[j]='\0';

	if(strcmp(cmd,"list")!=0 && strncmp(cmd,"get",3)!=0)
		continue;
	if (rttinit == 0) {
		rtt_init(&rttinfo); /* first time we're called */
		rttinit = 1;
		rtt_d_flag = 1;
	}

//------------------send command to the server--------------------------

	struct message msg;
	msg.sno=1;
	sprintf(msg.info_type,"CMD");
	sprintf(msg.type,"RQST");
	msg.last=1;
	sprintf(msg.msg,"%s",cmd);
	msg.ts = rtt_ts(&rttinfo);
	printf("\nsending command %s\n",msg.msg);
	int n=write(clientfd,(void*)&msg,sizeof(struct message));
	if (n > 0)
		printf("command sent\n");


//------------wait for ACK and child server port from parent server------------------
	
	signal(SIGALRM, connect_alarm);
	rtt_newpack(&rttinfo);
	alarm(rtt_start(&rttinfo));
	struct message reply;
		

	fd_set rset,wset;
	int maxfdp1;
	while(1)
	{
		FD_ZERO(&rset);
        	FD_SET(clientfd, &rset);
        	maxfdp1 = clientfd + 1;
		printf("\nwaiting on server to reply\n");
		select(maxfdp1, &rset, NULL, NULL, NULL);
		//printf("control is here\n");
		if (FD_ISSET(clientfd, &rset))
		{
			//printf("control is here 2\n");
			if(errno == EINTR)
			{
				if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n");
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return (-1);
                                }
                                msg.ts = rtt_ts(&rttinfo);
                                printf("resending\n");
                                n=write(clientfd,(void*)&msg,sizeof(struct message));
                                alarm(rtt_start(&rttinfo));
				continue;
			}
			int n=read(clientfd,(void *)&reply,sizeof(struct message));
			printf("n=%d errno=%d\n",n,errno);
			if(n<0 && errno==EINTR)
			{
                		continue;
			}
        		else if(n<=0)
        		{
                		printf("file client::%s::Server refused connection",args[2]);
                		return 0;
        		}
			else if(strcmp(reply.info_type,"CMD")==0 && strcmp(reply.msg,msg.msg)==0 && strcmp(reply.type,"ACK")==0)
			{
				break;
			}
		}
	}
	alarm(0);

	rtt_stop(&rttinfo, rtt_ts(&rttinfo) - reply.ts);

//--------------send START command to child server--------------------------------


	struct sockaddr_in childsrvaddr,testsrvaddr;
	memset(&childsrvaddr,0,sizeof(childsrvaddr));
	childsrvaddr.sin_family=AF_INET;
	childsrvaddr.sin_port=reply.server_port;

	if(inet_pton(AF_INET, (args[1]), &childsrvaddr.sin_addr) < 0)
	{
        	printf("file client::%s:: wrong address format",args[2]);
	}

	consts=connect(clientfd, (struct sockaddr *)&childsrvaddr, sizeof(childsrvaddr));
	printf("consts: %d\n",consts);
	socklen_t childsrvlen=sizeof(childsrvaddr);
        getsockname(clientfd,(struct sockaddr *)&testsrvaddr,&childsrvlen);
	printf("child server port %d\n",childsrvaddr.sin_port);
	msg.sno=1;
        sprintf(msg.info_type,"CMD");
        sprintf(msg.type,"RQST");
        msg.last=1;
        sprintf(msg.msg,"START");
	msg.ts = rtt_ts(&rttinfo);
	msg.bufspace=freebufspace;
        n=write(clientfd,(void*)&msg,sizeof(struct message));
	printf("write status: %d\n",n);
	signal(SIGALRM, connect_alarm);
        rtt_newpack(&rttinfo);
        alarm(rtt_start(&rttinfo));
	FD_ZERO(&rset);


//-----------------wait for ACK from child server-----------------------------------------

	while(1)
        {
                FD_ZERO(&rset);
                FD_SET(clientfd, &rset);
                maxfdp1 = clientfd + 1;
                printf("\nwaiting on child server to send ack\n");
                select(maxfdp1, &rset, NULL, NULL, NULL);
                //printf("control is here\n");
                if (FD_ISSET(clientfd, &rset))
                {
                        //printf("control is here 2\n");
                        if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n")
;
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return (-1);
                                }
                                msg.ts = rtt_ts(&rttinfo);
                                printf("resending start command\n");
                                n=write(clientfd,(void*)&msg,sizeof(struct message));
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }
                        int n=read(clientfd,(void *)&reply,sizeof(struct message));
                        printf("n=%d errno=%d\n",n,errno);
                        if(n<0 && errno==EINTR)
                        {
                                continue;
                        }
                        else if(n<=0)
                        {
                                printf("file client::%s::Server refused connection",args[2]);
                                return 0;
                        }
                        else if(strcmp(reply.info_type,"CMD")==0 && strcmp(reply.msg,msg.msg)==0 && strcmp(reply.type,"ACK")==0)
                        {
				printf("child %s ack recieved\n",reply.msg);
                                break;
                        }
                }
        }

	alarm(0);
	rtt_stop(&rttinfo, rtt_ts(&rttinfo) - reply.ts);
//------------------------------------recieve data from child server-----------------------------

	rtt_newpack(&rttinfo);
        alarm(rtt_start(&rttinfo));
        FD_ZERO(&rset);
        int index=1,ack_index=0;
	struct message ack;
	ack.sno=index - 1;
        sprintf(ack.info_type,"DATA");
        sprintf(ack.type,"ACK");
        ack.last=1;
        sprintf(ack.msg,"NULL");
	ack.ts=rtt_ts(&rttinfo);
	if(red==1)
        	fp=fopen(redirection,"w");
        else
                fp=fopen(redirection,"a+");
	while (reply.last != 1)
	{
		FD_SET(clientfd, &rset);
        	maxfdp1 = clientfd + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);
                if (FD_ISSET(clientfd, &rset))
                {
			if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n");
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return (-1);
                                }
                                printf("resending\n");
                                n=write(clientfd,(void*)&ack,sizeof(struct message));
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }
                        int n=read(clientfd,(void *)&reply,sizeof(struct message));
			//printf("%d %s\n",reply.last,reply.msg);
                        if(n<0 && errno==EINTR)
                                continue;
                        else if(n<=0)
                        {
                                printf("file client::%s::Server closed unexpectedly",args[2]);
                                break;
                        }
			else if(reply.sno==index)
			{
				if(strcmp(cmd,"list")==0)
				{
					printf(ANSI_COLOR_YELLOW "%d. %s\n" ANSI_COLOR_RESET,index,reply.msg);
					index++;
				}
				else 
				{
					if(((bufend + 1)%bufsize)!=bufstart)
					{
						buffer[bufend]=reply;
						bufend=(bufend + 1)%bufsize;
						index++;
						
					}
				}
                                ack.sno=index - 1;
                                sprintf(ack.info_type,"DATA");
                                sprintf(ack.type,"ACK");
                                ack.last=1;
				ack.ts=reply.ts;
                                sprintf(ack.msg,"%s",reply.msg);
				freebufspace=abs(((bufsize + 1)-bufend)-((bufsize + 1)-bufstart) -1);
				ack.bufspace=freebufspace;
                                n=write(clientfd,(void*)&ack,sizeof(ack));
                                if(n > 0)
                                {
                                        //printf("\nACK %d sent\n",index);
                                        ack_index=index - 1;
                                }
			}
		}
	}
	alarm(0);
	while(ack_index < (index - 1))
	{
                ack.sno=index - 1;
                sprintf(ack.info_type,"DATA");
                sprintf(ack.type,"ACK");
                ack.last=1;
                printf(ack.msg,"%s",reply.msg);
                n=write(clientfd,(void*)&ack,sizeof(ack));
                if(n > 0)
                {
                	printf("\nACK %d sent\n",index);
                        ack_index=index - 1;
                }
	}
	if (fp)
		fclose(fp);
	//printf(ANSI_COLOR_BLUE "file is: %s\n" ANSI_COLOR_RESET,str2);

}

close(clientfd);

exit(0);

}
