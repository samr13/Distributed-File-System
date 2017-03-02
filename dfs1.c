/* Author Shyam sundar Ramamoorthy <shra3971@colorado.edu
*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <dirent.h>

#define CONNCTIONS 50
#define TIMEOUT 10 
fd_set set;
char dirpath[25];
char defaultfile[5][25];
char defaultport[6]; 
int acc[CONNCTIONS];
int serverid;
struct timeval timeout;
int conn_estb=0;
struct {
    char username[15];
    char password[15];
} dfs[CONNCTIONS];


/* Sets timeout value for socket*/
int set_timer (int sockfd,int clientid)
{
    FD_SET (sockfd, &set);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    return (select (getdtablesize(), &set, NULL, NULL, &timeout));
}

/* Creates a socket, listen at the port and returns socket descriptor*/
int getSocketId(int port) {
	int s=0,yes=1;
	if((s=socket(AF_INET,SOCK_STREAM,0))==-1) {
		perror("server socket");
	}
    //setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	
    struct sockaddr_in addr ;
	addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	addr.sin_port=htons(port);
	addr.sin_family=PF_INET;
	bzero(&addr.sin_zero,8);
	if (bind(s,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))==-1) {
		perror("bind error");
		exit(1);
	} else {
		printf ("binded to ipaddr: 127.0.0.1 & port: %d\n",port );
	}

     if((listen(s,CONNCTIONS))==-1) {
         perror ("listen error");
     }
	
	return s;
}

/* parse given dfs.conf and writes in a global structure*/
void pwd_validation (int clientid) {
     FILE* fd=fopen("dfs.conf","r");
     char *lin=NULL, credentials[50], *passed_username, *passed_pwd, *user, *pwd;
     int match=0,leng_recv;
     size_t len=0;
     ssize_t read;
     int port;
     if(fd==NULL) {
        printf ("configuration file not found");
        exit(0);
     }
    
    if ((leng_recv=recv(acc[clientid],credentials, sizeof(credentials)-1, 0))!=0) {
        credentials[leng_recv]='\0';
        //printf("Got : %s %d\n", credentials,leng_recv);
        //match=pwd_validation(credentials, clientid);
    } else {
        perror("No (credentials) from client");
        exit(0);
    }           
 
     passed_username=strtok(credentials,"\t");
     passed_pwd= strtok(NULL,"\t\n");

     while ((read=getline(&lin, &len,fd))!=-1) {
         user=strtok(lin," ");
         pwd=strtok(NULL,"\t\n");
        //printf("Passed: %sE %sE FromFile %sE %sE\n",passed_username, passed_pwd, user, pwd); 
        if (strcmp(passed_username, user)==0) {
            if (strcmp(passed_pwd, pwd)==0) {
                //printf("Credentials matched for user %s \n",user);
                match=1;
                strcpy(dfs[clientid].username,user);
                strcpy(dfs[clientid].password,pwd);
                send(acc[clientid], "correct", 7, 0);
                sprintf(dirpath,"%s%s%s",dirpath,dfs[clientid].username,"/");
            } else {
                printf("Password not match for %s\n", user);
            }
            break;
        }
    }
    
    if (match == 0) {
        send(acc[clientid], "wrong", 5, 0);
        sleep(1);
        shutdown(acc[clientid],SHUT_RDWR);
        close(acc[clientid]);
        printf("\n%dth client closed because user credentials failed\n",clientid);
    }     
}

void putprocess (int clientid,char* file) {
    struct stat st = {0};
    //printf("Dir path%s\n",dirpath );
    if (stat(dirpath, &st) == -1) {
        mkdir(dirpath, 0700);
    }
    strcpy(file,dirpath);
    sleep(1);
    //strcat(file,strtok(NULL,"\n"));
    int kk=1;
    char recvBuff[2]="";
    char str[80],decfile[30];

    while (kk<3) {
        FILE* fd;
        int n;
        char filename[25];
        memset(filename,0, sizeof(filename));
        //fflush(NULL);
        sleep(1);
        if ((n=recv(acc[clientid], filename, sizeof(filename), 0))>0) {
            if(filename[0]=='\0' ||filename[0]=='|')
                filename[0]='/';
            filename[n]='\0';
            //printf("file name%s %d\n", filename, n);
            if ((send(acc[clientid], "ack", 3, 0)<=0)) {
                printf("Not send ack\n");
            }
        } else {
            printf("Error reading filename\n");
        }
        strcpy(decfile,file);
        strcat(decfile,"dec/");    
        struct stat st1 = {0};
        //printf("Dir path%s\n",dirpath );
        if (stat(decfile, &st1) == -1) {
            mkdir(decfile, 0700);
        }
        //printf("\nFile to be written :%s\n", file);
        //sprintf(decfile,"%sdecryptfolder/%s_dec",file,filename);
        strcat(file,filename);
        
        strcat(decfile,filename);
        strcat(decfile,"_d");
        //printf("dec file: %s\n file %s\n", decfile,file);
        if((fd=fopen(decfile,"w+"))!=NULL) {
            sleep(1);
            //printf("contents:\n");
            while ( (n = recv(acc[clientid], recvBuff, sizeof(recvBuff)-1,0)) > 0)
            {   recvBuff[n] = '\0';
                if(strncmp(recvBuff,"|",1)==0) 
                    break;
                else {
                        fputc(recvBuff[0],fd);
                        printf("%s",recvBuff);
                }
            } 
            //printf("Finished receiving\n");
            printf("\nFile written :%s\n", file);
            fclose(fd);
            if ((send(acc[clientid], "ack", 3, 0)<=0)) {
                printf("Not send ack\n");
            }
            //printf("dec file: %s\n file %s\n", decfile,file);
            sprintf(str,"openssl base64 -d -in %s -out %s",decfile,file); 
            FILE *dec= popen(str, "r");

            strcpy(file,dirpath);
        } else {
            printf("No file opened\n");
            break;
        }
        kk=kk+1;
    }
}

void listprocess (int clientid) {
    DIR *dir;
    struct dirent *ent;
    char filename[25];
    char ack[4];
    int n;
    //printf("Dir path: %s\n", dirpath);
    if ((dir = opendir (dirpath)) != NULL) {
        //printf("\nFiles Listed:");
        while ((ent = readdir (dir)) != NULL) {
            strcpy(filename,ent->d_name);
            
            if ((strcmp(filename,"."))!=0 && (strcmp(filename,".."))!=0 && (strncmp(filename," ",1))!=0 ) {
                //printf ("%s", filename);
                send(acc[clientid],filename,sizeof(filename),0);
                if ((n=recv(acc[clientid], ack, sizeof(ack)-1,0)<=0))  {
                    printf("Ack not received\n");
                } else {
                    ack[n]='\0';
                }
                //sleep(1);
            }
        }
        send(acc[clientid],"end",3,0);
        fflush(NULL);
        closedir (dir);
    } else {
        printf("No folder %s exists for username %s\n", dirpath ,dfs[clientid].username);
        send(acc[clientid],"end",3,0);  
    }

}

void getprocess (int clientid, char *file) {
    int f, i,j, count,len, n;
    char buf[1], ack[4], filename[40], temp[25];
    strncpy(temp,file,24);
    temp[24]='\0';
    //printf("sockfd %d filename %s\n",acc[clientid], file );
    fflush(NULL);
    //sleep(2);
    for (j=1; j<5; j++) {
        char splitfilename[25];
        sprintf(splitfilename, "%s%s%s%d",".",temp,".",j);
        strcpy(filename,dirpath);
        strcat(filename,splitfilename);
        

        if ((f=open(filename,O_RDONLY))!=-1) { 
            if ((n=send(acc[clientid], splitfilename, sizeof(splitfilename), 0))<=0) {
                printf("Not sent filename\n");
            } else {
                //printf("send filename AA%sBB %d\n", file, n );
                if ((n=recv(acc[clientid], ack, sizeof(ack)-1,0))<=0) {
                    printf("Not received ack\n");
                } else {
                    ack[n]='\0';
                    //printf("Ack char %d %sEE \n",n, ack );
                    if(strcmp(ack,"end")==0) {
                        printf("Negative ack (not needed by client) for %s\n", filename);
                        continue;
                    } else {
                        printf("File to be sent : %s\n", filename);
                        while ((count=read(f, buf, sizeof(buf)))>0 ) {
                            len=send(acc[clientid], buf, count,0);
                            //printf("sent: %s\n", buf );
                        }
                        send(acc[clientid], "|", 1,0);
                        //printf("Finished sending\n");
                        if ((n=recv(acc[clientid], ack, sizeof(ack)-1,0))<=0) {
                                printf("Not received ack\n");
                        } else {
                            ack[n]='\0';
                            //printf("recvd ack after content transfer, recvd:%s\n ",ack);
                        }
                    }
                }
            }
        } else {
            //printf("Split file not found %d\n",j);
        }
        close(f);
    }
}
/* wait for request and returns a response with timer implementation */
int dsresponse (int clientid) {
	int len=0,count=0,f,select_out,leng_recv=0,j=0, match;
	char str[31],buf[1024],file[24], file1[50], *option, *subfolder;
    while (1) {
            set_timer(acc[clientid],clientid);
            
            if (FD_ISSET(acc[clientid],&set) ) {
                //printf ("\nInside  %dth client: FD_ISSET    %d\n",clientid,FD_ISSET(acc[clientid],&set));
                memset(str,0, sizeof(str));

                if ((leng_recv = recv(acc[clientid],str, sizeof(str)-1, 0))>0) {
                    str[leng_recv]='\0';
                    //printf("leng_recv%d\n", leng_recv);
                    if (strcmp(str,"ack")!=0) {
                        printf ("Message received: %s ",str);
                        send(acc[clientid], "ack", 3, 0);
                        printf("ack sent\n");
                    }
                } else {
                    perror("No further Message(option) from client");
                    exit(0);
                }
                fflush(NULL);
     
                if (leng_recv == 0) {
	                shutdown(acc[clientid],SHUT_RDWR);
                    close(acc[clientid]);
                    FD_CLR(acc[clientid],&set);
                    printf("\n%dth client closed : No data received \n",clientid);
                } else {
                    option=strtok(str," ");
                    if (strcmp(option,"GET")==0 || strcmp(option,"get")==0) {
                        strcpy(file,strtok(NULL," "));   
                        if((subfolder=strtok(NULL,"/"))!=NULL ) { 
                            strcpy(dirpath,strtok(dirpath,"/"));
                            sprintf(dirpath,"%s%s%s%s",dirpath,"/",subfolder,"/");  
                            //printf("Got subfolder\n");           
                        }  
                        //printf("dirpath %s\n", dirpath);
                        listprocess(clientid);
                        getprocess(clientid,file);
                    } else if (strcmp(option,"PUT")==0 || strcmp(option,"put")==0) {
                        strcpy(file,strtok(NULL," "));
                        if((subfolder=strtok(NULL,"/"))!=NULL ) { 
                            strcpy(dirpath,strtok(dirpath,"/"));
                            sprintf(dirpath,"%s%s%s%s",dirpath,"/",subfolder,"/");
                            //printf("Got subfolder\n");
                        }
                        //printf("dirpath %s\n", dirpath);                                     
                        putprocess(clientid, file1);
                    } else if (strcmp(option,"LIST")==0 || strcmp(option,"list")==0) {
                        if((subfolder=strtok(NULL,"/"))!=NULL ) { 
                            strcpy(dirpath,strtok(dirpath,"/"));
                            sprintf(dirpath,"%s%s%s%s",dirpath,"/",subfolder,"/"); 
                            //printf("Got subfolder\n");           
                        }     
                        //printf("dirpath %s\n", dirpath);                     
                        listprocess(clientid);
                    } else if (strcmp(option,"0")==0) {
                        shutdown(acc[clientid],SHUT_RDWR);
                        close(acc[clientid]);
                        FD_CLR(acc[clientid],&set);
                        printf("\n%dth client initiated close connection\n",clientid);
                    } else {
                        printf("Invalid option");
                    }
                    strcpy(dirpath,strtok(dirpath,"/"));
                    strcat(dirpath,"/");
                    sprintf(dirpath,"%s%s%s",dirpath,dfs[clientid].username,"/");
                    //printf("resetted to %s\n", dirpath);
                }
         
            } else {
	           shutdown(acc[clientid],SHUT_RDWR);
               close(acc[clientid]);
               FD_CLR(acc[clientid],&set);
               printf("\n%dth client closed after timeout\n",clientid);
               return 1;
            }
    }
}


int main(int argc, char *argv[]) {

	socklen_t siz_sock=sizeof(struct sockaddr_in);
	int clientid=0,resp=1,num_conn=0;
    long z;
	int i,j=0,k=0,port=10001;
    //pwd_validation();

    if (argc!=3) {
        printf("Usage: ./file <DIR> <PORT>\n\n");
        exit(0);
    } else {
        strcpy(dirpath,argv[1]);
        serverid=dirpath[3];
        strcat(dirpath,"/");
        port=atoi(argv[2]);
    }
    
    //printf("port %d\n", port);
    acc[0]=getSocketId(port);
    FD_ZERO (&set);
    //printf ("Server socketid:%d\n",acc[0]);
	
    while (1) {
        if ((acc[++conn_estb]=accept(acc[0],(struct sockaddr*)NULL,&siz_sock))==-1) {
	      printf("accept failed");
	    } else {
	        //printf ("Accept id: %d \n",acc[conn_estb]);
            int f=fork();
            if ( f==0 ) { 
                    pwd_validation(conn_estb);
                    resp=dsresponse(conn_estb);
                    strcpy(dirpath,argv[1]);
                    strcat(dirpath,"/");                    
            }
         }
	}
    
}
