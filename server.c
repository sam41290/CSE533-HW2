/*-----------------------------------------------------------------------------------------------------------------//
        Creator: Soumyakant Priyadarshan                                                                           //
        SBU ID: 111580387                                                                                          //
        email: spriyadarsha@cs.stonybrook.edu                                                                      //
                                                                                                                   //
   References:                                                                                                     //
        1. https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c                              //
        2. http://www.geeksforgeeks.org/socket-programming-cc/                                                     //
        3. http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html                                            //
        4. https://stackoverflow.com/questions/3437404/min-and-max-in-c                                            //
        5. Addison Wesley : UNIX Network Programming Volume 1, Third Edition: The Sockets Networking API           //
-------------------------------------------------------------------------------------------------------------------*/

#include<stdio.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include<errno.h>
#include<signal.h>
#include<sys/time.h>
#include<sys/select.h>
#include<time.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/types.h>
#include"mytypes.h"
#include"unprtt.h"
#define FILE_PORT 8080
#define MAX(x, y) (((x) > (y)) ? (x) : (y))


static struct rtt_info rttinfo;
static int rttinit = 0;


static void connect_alarm(int signo)
{
        //printf("alarm\n");
        return;
}

void sendlist(int childlistenfd,struct sockaddr_in filecliaddr,socklen_t fileaddrlen)
{
	if (rttinit == 0) {
                rtt_init(&rttinfo); /* first time we're called */
                rttinit = 1;
                rtt_d_flag = 1;
        }


	struct message reply,request;
	fd_set rset;
        int maxfdp1;

	signal(SIGALRM, connect_alarm);
        rtt_newpack(&rttinfo);
        alarm(rtt_start(&rttinfo));

        while(1)
        {
		printf("in child\n");
		FD_ZERO(&rset);
        	FD_SET(childlistenfd, &rset);
        	maxfdp1 = childlistenfd + 1;
        	select(maxfdp1, &rset, NULL, NULL, NULL);
                if (FD_ISSET(childlistenfd, &rset))
                {
			
			if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n")
;
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return;
                                }
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }

			printf("client signal received\n");
                	int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
			printf("%s\n",request.msg);

                        if(n<0 && errno==EINTR)
                        	continue;
                        else if(n<=0)
                        {
                                printf("file server::::client closed unexpectedly");
                                return;
                        }
                        else if(strcmp(request.info_type,"CMD")==0 && strcmp(request.msg,"START")==0 && strcmp(request.type,"RQST")==0)
                        {
				printf("start command received\n");
				reply.sno=1;
                                sprintf(reply.info_type,"CMD");
                                sprintf(reply.type,"ACK");
                                reply.last=0;
				reply.ts=request.ts;
                                sprintf(reply.msg,"%s",request.msg);
				sendto(childlistenfd, (void *)&reply, sizeof(struct message), 0, (struct
 sockaddr *)&filecliaddr, fileaddrlen);
                                break;
                        }
                 }
         }

	 alarm(0);
         struct message dirlst;
	 struct message bkp[1024];
         DIR  *d;
         struct dirent *dir;
         d = opendir(".");
         if (d)
         {
         	int index=1;
                dir = readdir(d);
		int ack=0;
                while (dir!= NULL)
                {
			fd_set rset;
        		fd_set wset;
        		FD_ZERO(&rset);
        		FD_ZERO(&wset);
			
                	FD_SET(childlistenfd,&rset);
                	FD_SET(childlistenfd,&wset);
                	int maxfdp1=childlistenfd + 1;
                	select(maxfdp1, &rset,&wset,NULL,NULL);
                	if (FD_ISSET(childlistenfd,&rset))
                	{
				int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message),0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
                        	if(n > 0 && strcmp(request.type,"ACK")==0)
                        	{
                                	ack=request.sno;
                                	printf("\nACK %d received\n",ack);
                        	}else printf("\n%d",errno);
			}
			if (FD_ISSET(childlistenfd,&wset))
                        {
				printf("%s\n",dir->d_name);
                		sprintf(dirlst.msg,"%s", dir->d_name);
                        	sprintf(dirlst.info_type,"DATA");
                        	sprintf(dirlst.type,"MSG");
                        	dirlst.sno=index;
                        	dir = readdir(d);
                        	if (dir == NULL)
                                  	dirlst.last=1;
                        	else dirlst.last=0;
				bkp[index - 1]=dirlst;
				index++;
				dirlst.ts=rtt_ts(&rttinfo);
                        	sendto(childlistenfd, (void *)&dirlst, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
			}
                }
		rtt_newpack(&rttinfo);
        	alarm(rtt_start(&rttinfo));
		while (ack < (index - 1))
		{
			int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message),0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
			if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n")
;
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return;
                                }
                                printf("resending\n");
				alarm(0);
				int i=ack + 1;
				while (i < index)
				{
					sendto(childlistenfd, (void *)&bkp[i], sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
					i++;
				}
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }
                        if(n > 0 && strcmp(request.type,"ACK")==0)
                        {
				alarm(rtt_start(&rttinfo));
                                ack=request.sno;
				printf("\nACK %d received\n",ack);
                        }//else printf("\n%d",errno);
		}
		printf("listing complete\n");
                closedir(d);
         }

}



void sendfile(int childlistenfd,struct sockaddr_in filecliaddr,socklen_t fileaddrlen,char cmd[100])
{

	int i=0;
	int j=0;
	char file[50];
	int t=0;
	while(cmd[i]!='\0')
	{
		if(cmd[i]==' ')
			t=1;
		if(t==1)
		{
			if(cmd[i]!=' ')
			{
				file[j]=cmd[i];
				j++;
			}				
		}
		i++;
	}
	file[j]='\0';
	
	if (rttinit == 0) {
                rtt_init(&rttinfo); /* first time we're called */
                rttinit = 1;
                rtt_d_flag = 1;
        }


	struct message reply,request;
	fd_set rset;
        int maxfdp1;

	signal(SIGALRM, connect_alarm);
        rtt_newpack(&rttinfo);
        alarm(rtt_start(&rttinfo));

        while(1)
        {
		printf("in child\n");
		FD_ZERO(&rset);
        	FD_SET(childlistenfd, &rset);
        	maxfdp1 = childlistenfd + 1;
        	select(maxfdp1, &rset, NULL, NULL, NULL);
                if (FD_ISSET(childlistenfd, &rset))
                {
			
			if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n");
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return;
                                }
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }

			printf("client signal received\n");
                	int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
			printf("%s\n",request.msg);

                        if(n<0 && errno==EINTR)
                        	continue;
                        else if(n<=0)
                        {
                                printf("file server::::client closed unexpectedly");
                                return;
                        }
                        else if(strcmp(request.info_type,"CMD")==0 && strcmp(request.msg,"START")==0 && strcmp(request.type,"RQST")==0)
                        {
				printf("start command received\n");
				reply.sno=1;
                                sprintf(reply.info_type,"CMD");
                                sprintf(reply.type,"ACK");
                                reply.last=0;
				reply.ts=request.ts;
                                sprintf(reply.msg,"%s",request.msg);
				sendto(childlistenfd, (void *)&reply, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
                                break;
                        }
                 }
         }

	 alarm(0);
         struct message data;
	 struct message bkp[1024];
         FILE  *fp;
	 printf("file: %s\n",file);
         fp = fopen(file,"r");
         if (fp)
         {
         	int index=1;
		int ack=0;
		char *buf=malloc(sizeof(char)*100);
		buf=fgets(buf,100,fp);
                while (buf!=NULL)
                {
			fd_set rset;
        		fd_set wset;
        		FD_ZERO(&rset);
        		FD_ZERO(&wset);
			
                	FD_SET(childlistenfd,&rset);
                	FD_SET(childlistenfd,&wset);
                	int maxfdp1=childlistenfd + 1;
                	select(maxfdp1, &rset,&wset,NULL,NULL);
                	if (FD_ISSET(childlistenfd,&rset))
                	{
				int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message),0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
                        	if(n > 0 && strcmp(request.type,"ACK")==0)
                        	{
                                	ack=request.sno;
                                	printf("\nACK %d received\n",ack);
                        	}else printf("\n%d",errno);
			}
			if (FD_ISSET(childlistenfd,&wset))
                        {
                		sprintf(data.msg,"%s", buf);
                        	sprintf(data.info_type,"DATA");
                        	sprintf(data.type,"MSG");
                        	data.sno=index;
                        	buf=fgets(buf,100,fp);
                        	if (buf == NULL)
                                  	data.last=1;
                        	else data.last=0;
				bkp[index - 1]=data;
				index++;
				data.ts=rtt_ts(&rttinfo);
                        	sendto(childlistenfd, (void *)&data, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
			}
                }
		rtt_newpack(&rttinfo);
        	alarm(rtt_start(&rttinfo));
		while (ack < (index - 1))
		{
			int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message),0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
			if(errno == EINTR)
                        {
                                if (rtt_timeout(&rttinfo) < 0)
                                {
                                        printf("\nfile client:::: no response from server, giving up\n");
                                        rttinit = 0; /* reinit in case we're called again */
                                        errno = ETIMEDOUT;
                                        return;
                                }
                                printf("resending\n");
				alarm(0);
				int i=ack + 1;
				while (i < index)
				{
					sendto(childlistenfd, (void *)&bkp[i], sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
					i++;
				}
                                alarm(rtt_start(&rttinfo));
                                continue;
                        }
                        if(n > 0 && strcmp(request.type,"ACK")==0)
                        {
				alarm(rtt_start(&rttinfo));
                                ack=request.sno;
				printf("\nACK %d received\n",ack);
                        }//else printf("\n%d",errno);
		}
		printf("listing complete\n");
                fclose(fp);
         }

}

int main(int argc,char *args[])
{

	chdir("./DIR");	
	int filelistenfd=socket(AF_INET,SOCK_DGRAM,0);

	if (filelistenfd < 0 )
	{
		printf("socket() failed\n");
		return 0;
	}

	struct sockaddr_in filecliaddr;
	struct sockaddr_in filesrvaddr;
	int fileaddrlen=sizeof(filecliaddr);

	memset(&filesrvaddr,0,sizeof(filesrvaddr));
	memset(&filecliaddr,0,sizeof(filecliaddr));

	filesrvaddr.sin_family=AF_INET;
	filesrvaddr.sin_port=htons(FILE_PORT);
	filesrvaddr.sin_addr.s_addr = htonl(INADDR_ANY);



	if (bind(filelistenfd, (struct sockaddr *)&filesrvaddr,sizeof(filesrvaddr))<0)
	{
        	printf("file bind failed");
        	return 0;
	}


//printf("\nwaiting on listen");

	
	int n;
	struct message request,reply;
	for ( ; ; ) 
	{
		printf("waiting for client\n");
		n = recvfrom(filelistenfd,(void *)&request, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
		//if(n > 0)
			printf("command recieved\n");
		
		if(strcmp(request.info_type,"CMD")==0 && (strcmp(request.msg,"list")==0 || strncmp(request.msg,"get",3)==0))
		{
			sprintf(reply.msg,"%s",request.msg);
			sprintf(reply.info_type,"%s",request.info_type);
			sprintf(reply.type,"ACK");
			reply.sno=1;
			reply.last=0;
			struct sockaddr_in childsrvaddr;
			int childlistenfd=socket(AF_INET,SOCK_DGRAM,0);
			memset(&childsrvaddr,0,sizeof(childsrvaddr));
        		childsrvaddr.sin_family=AF_INET;
        		childsrvaddr.sin_addr.s_addr = htonl(INADDR_ANY);			
			if (bind(childlistenfd, (struct sockaddr *)&childsrvaddr,sizeof(childsrvaddr))<0)
        		{
                		printf("file bind failed");
                		return 0;
        		}
			socklen_t childsrvlen=sizeof(childsrvaddr);
			getsockname(childlistenfd,(struct sockaddr *)&childsrvaddr,&childsrvlen);
			printf("child port %d\n", childsrvaddr.sin_port);
			reply.server_port= childsrvaddr.sin_port;
			reply.ts=request.ts;
			sendto(filelistenfd, (void *)&reply, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
			pid_t pid=fork();
			if(pid == 0)
			{
				if(strcmp(request.msg,"list")==0)
					sendlist(childlistenfd,filecliaddr,fileaddrlen);
				else
					sendfile(childlistenfd,filecliaddr,fileaddrlen,request.msg);
				exit(0);
			}
			else
			{
				int status;
				pid_t p=waitpid(pid,&status,WNOHANG);
			}
		}	
 	}


//printf("\nlisten complete");
//	signal(SIGCHLD,sig_chld);
	return 0;

}
