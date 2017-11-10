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
#define MAX(x, y) (((x) > (y)) ? (x) : (y))



int srv_port;
int maxbufsize;
char *dir;

static void connect_alarm(int signo)
{
        //printf("alarm\n");
		//printf("timeout\n");
        return;
}

char *getlist(DIR *d,char *buf)
{
	struct dirent *dir;
	dir=readdir(d);
	if(dir == NULL)
		return NULL;
	sprintf(buf,"%s",dir->d_name);
	return buf;
}


void senddata(int childlistenfd,struct sockaddr_in filecliaddr,socklen_t fileaddrlen,char cmd[100])
{

	int i=0;
	int j=0;
	char file[50];
	int t=0;
	
//--------------------------Parse command to get file name-------------------------------------//
	
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

//----------------------------------------------------------------------------------------------//

	static struct rtt_info rttinfo;
	static int rttinit = 0;
	if (rttinit == 0) {
                rtt_init(&rttinfo); /* first time we're called */
                rttinit = 1;
                rtt_d_flag = 1;
        }


	struct message reply,request;
	fd_set rset;
    int maxfdp1;

//------------------------------Wait for start command from client--------------------------------//	
	
	signal(SIGALRM, connect_alarm);
    rtt_newpack(&rttinfo);
    alarm(rtt_start(&rttinfo));
    
    while(1)
    {
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
							printf("\nfile server123:::: no response from client, giving up\n");
							rttinit = 0; /* reinit in case we're called again */
							errno = ETIMEDOUT;
							return;
					}
					//printf("timeout 1\n");
					alarm(rtt_start(&rttinfo));
					errno=0;
					continue;
				}
		
				//printf("client signal received\n");
				int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
						//printf("%s\n",request.msg);
		
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
						break;
				}
            }
    	}
	//rtt_stop(&rttinfo, rtt_ts(&rttinfo) - request.ts);
	alarm(0);
//------------------------START command received send ACK----------------------------------------------------//
	SEND_ACK:
	printf("START ACK sent\n");
	reply.sno=1;
    sprintf(reply.info_type,"CMD");
    sprintf(reply.type,"ACK");
    reply.last=0;
    reply.ts=request.ts;
    sprintf(reply.msg,"%s",request.msg);
    sendto(childlistenfd, (void *)&reply, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);


//-----------------------------------------------------------------------------------------------------------//

//-------------------------Start data transfer----------------------------------------------------------------//


	int clientbuf=request.bufspace;
    struct message data;
	struct message *bkp;
	int bufsize=(maxbufsize/3);
	bkp=malloc(sizeof(struct message)*(bufsize+1));
	printf("\nInitial buffer size %d\n",bufsize);
	int bufstart=0,bufend=0;
    void  *fp;
	if(strcmp(cmd,"list")==0)
		fp=(DIR *)opendir(".");
	 //printf("file: %s\n",file);
	else if(strncmp(cmd,"get",3)==0)
    fp =(FILE *)fopen(file,"r");
    if (fp)
    {
         	int index=1;
			int ack=0;
			char *buf=malloc(sizeof(char)*100);
			if(strcmp(cmd,"list")==0)
                	buf=getlist(fp,buf);
			//printf("file: %s\n",file);
         	else if(strncmp(cmd,"get",3)==0)
				buf=fgets(buf,100,fp);

//------------------continure till all data is sent and all ACK is received-----------------------------------

            while (buf!=NULL || bufend!=bufstart) 
            {
					printf("server bufstart %d server bufend %d clientbuf %d ||sending data index %d\n",bufstart,bufend,clientbuf,index);


					if(((bufend + 1)%(bufsize+1))==bufstart || buf==NULL || clientbuf <=0 )    //If server buffer is full..wait for client ACK for all sent packets
					{
						printf("in waiting zone\n");
						rtt_newpack(&rttinfo);
                		alarm(rtt_start(&rttinfo));
                		while (bufend != bufstart)
                		{
						//printf("stuck in here\n");
                        		int n=recvfrom(childlistenfd,(void *)&request, sizeof(struct message),0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
                        		if(n>0 && strcmp(request.info_type,"CMD")==0 && strcmp(request.msg,"START")==0 && strcmp(request.type,"RQST")==0)
								{
									printf("resending ACK\n");
									goto SEND_ACK;
								}
								if(errno == EINTR)
                        		{
                                		if (rtt_timeout(&rttinfo) < 0)
                                		{
                                        		printf("\nfile server:::: no response from server, giving up\n");
                                        		rttinit = 0; /* reinit in case we're called again */
                                        		errno = ETIMEDOUT;
                                        		return;
                                		}
                                		
                                		alarm(0);
                                		int i=ack + 1;
										
										int ctr=bufstart;
                                		while (i < index)
                                		{
												printf("resending index %d\n",i);
                                        		sendto(childlistenfd, (void *)&bkp[ctr], sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);  //resending the packets from server buffer
                                        		i++;
												ctr=(ctr + 1)%(bufsize+1);
												alarm(rtt_start(&rttinfo));
                                		}
                                		alarm(rtt_start(&rttinfo));
										errno=0;
                                		continue;
                        		}
                        		if(n > 0 && strcmp(request.type,"ACK")==0)
                        		{
                                		alarm(rtt_start(&rttinfo));
										if(request.sno <=ack)
										{
											int i=request.sno + 1;
											int ctr=bufstart;
											while (i < index)
											{
												printf("resending index %d\n",i);
                                        		sendto(childlistenfd, (void *)&bkp[ctr], sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);  //resending the packets from server buffer
                                        		i++;
												ctr=(ctr + 1)%(bufsize+1);
												alarm(rtt_start(&rttinfo));
											}
										}
                                		ack=request.sno;
                                		printf("\nACK %d received %d\n",ack,request.bufspace);
										clientbuf=request.bufspace;
										while(bkp[bufstart].sno<=request.sno && bufstart != bufend)
                                        bufstart=(bufstart + 1)%(bufsize+1);
										if((bufstart == bufend)&&(bufsize < maxbufsize))//sliding window: increasing the buffer size
										{
											bufsize = bufsize* 2 ;
											if(bufsize > maxbufsize)
												bufsize=maxbufsize;
											bkp=malloc(sizeof(struct message)*(bufsize+1));
											printf("\nserver buffer size increased to %d\n",bufsize);
										}
										//printf("bufstart %d bufend %d clientbuf %d\n",bufstart,bufend,clientbuf);
                        		}//else printf("\n%d",errno);
								
                		}
				
					}
					if(buf==NULL)
						break;


//------------------------------------DATA sending module---------------------------------------------------------------//

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
						while(recvfrom(childlistenfd,(void *)&request, sizeof(struct message),MSG_DONTWAIT, (struct sockaddr *)&filecliaddr, &fileaddrlen) > 0)
						{
							printf("%s\n",request.msg);
							if(strcmp(request.info_type,"CMD")==0 && strcmp(request.msg,"START")==0 && strcmp(request.type,"RQST")==0)
								{
									printf("resending ACK\n");
									goto SEND_ACK;
								}
							if(strcmp(request.type,"ACK")==0)
							{
								ack=request.sno;
								printf("\nACK %d received %d\n",ack,request.bufspace);
								clientbuf=request.bufspace;
								while(bkp[bufstart].sno<=request.sno && bufstart != bufend)
								bufstart=(bufstart + 1)%(bufsize+1);
								if((bufstart == bufend)&&(bufsize < maxbufsize))//sliding window: increasing the buffer size
								{
									bufsize = bufsize * 2;
									if(bufsize > maxbufsize)
										bufsize=maxbufsize;
									bkp=malloc(sizeof(struct message)*(bufsize+1));
									printf("\nserver buffer size increased to %d\n",bufsize);
								}
							
							}//else printf("\n%d",errno);
						}
					}
					if (FD_ISSET(childlistenfd,&wset))
					{
						if(((bufend + 1)%(bufsize+1))!=bufstart)
						{
							sprintf(data.msg,"%s", buf);
							sprintf(data.info_type,"DATA");
							sprintf(data.type,"MSG");
							data.sno=index;
							//buf=fgets(buf,100,fp);
							if(strcmp(cmd,"list")==0)
								buf=getlist(fp,buf);
							//printf("file: %s\n",file);
							else if(strncmp(cmd,"get",3)==0)
								buf=fgets(buf,100,fp);
							if (buf == NULL)
								data.last=1;
							else data.last=0;
							data.ts=rtt_ts(&rttinfo);
							bkp[bufend]=data;
							bufend=(bufend + 1)%(bufsize + 1);
							index++;
							if(clientbuf > 0)
								sendto(childlistenfd, (void *)&data, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
						}
					}


//---------------------------------------------------------------------------------------------------------------------------------------//
			
            }
    }
	alarm(0);
	printf("data sent\n");
    //close(fp);
	return;
}

int main(int argc,char *args[])
{
	FILE *srv=fopen("./asgn2/server.in","r");
	if(srv==NULL)
        	printf("can not open server.in file");
	dir=malloc(sizeof(char)*100);
	fscanf(srv,"%d",&srv_port);
	fscanf(srv,"%d",&maxbufsize);
	fscanf(srv,"%s",dir);

	fclose(srv);

	chdir(dir);	
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
	filesrvaddr.sin_port=htons(srv_port);
	filesrvaddr.sin_addr.s_addr = htonl(INADDR_ANY);



	if (bind(filelistenfd, (struct sockaddr *)&filesrvaddr,sizeof(filesrvaddr))<0)
	{
        	printf("file server::::bind failed");
        	return 0;
	}


//printf("\nwaiting on listen");

	
	int n;
	struct message request,reply;
	for ( ; ; ) 
	{
		printf("\nwaiting for command\n");
		n = recvfrom(filelistenfd,(void *)&request, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, &fileaddrlen);
		//if(n > 0)
			printf("\ncommand recieved\n");
		
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
            	printf("file server::::child bind failed");
            	return 0;
        	}
			socklen_t childsrvlen=sizeof(childsrvaddr);
			getsockname(childlistenfd,(struct sockaddr *)&childsrvaddr,&childsrvlen);
			//printf("child port %d\n", childsrvaddr.sin_port);
			reply.server_port= childsrvaddr.sin_port;
			reply.ts=request.ts;
			sendto(filelistenfd, (void *)&reply, sizeof(struct message), 0, (struct sockaddr *)&filecliaddr, fileaddrlen);
			pid_t pid=fork();
			if(pid == 0)
			{
				senddata(childlistenfd,filecliaddr,fileaddrlen,request.msg);
				printf("child exit\n");
				exit(0);
			}
			else
			{
				int status;
				pid_t p=waitpid(pid,&status,WNOHANG);
			}
		}
		else printf("\nBad command\n");
 	}


//printf("\nlisten complete");
//	signal(SIGCHLD,sig_chld);
	return 0;

}
