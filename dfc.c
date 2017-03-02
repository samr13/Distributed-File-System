/* Author Shyam sundar Ramamoorthy <shra3971@colorado.edu */
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
#include <pthread.h>

#define TIMEOUT 2

int totalServers, clientid[10], indexSplit[5][4];
char splitarray[5][25];
char complete[5][15];
struct timeval timeout;
fd_set set;
char client_username[20];
char client_password[20];
char opt[30], hash[4];
struct {
	char name[10];
    char ip[15];
    long int port;
} dfc[10];


/* Sets timeout value for socket*/
int set_timer (int sockfd)
{
    FD_SET (sockfd, &set);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    return (select (getdtablesize(), &set, NULL, NULL, &timeout));
}

int parseDfc() {
    int  n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 
     FILE* fd=fopen("dfc.conf","r");
     char *lin=NULL,*pa,*line,*param ,*char_port;
     int i=0,j=0;
     size_t len=0;
     ssize_t read;
     
     if(fd==NULL) {
         printf ("client configuration file not found");
         exit(0);
     }
     while ((read=getline(&lin,&len,fd))!=-1) {
         if(lin==NULL) {
            break;
         }
         param=strtok(lin," ");

         if (strcmp(param,"Server")==0) {
             i++;            
             strcpy(dfc[i].name,strtok(NULL," "));
             strcpy(dfc[i].ip,strtok(NULL,":"));             
             char_port=strtok(NULL,"\n");
             dfc[i].port=atoi(char_port);
             //printf("name ip port:%s %s %ld\n",dfc[i].name,dfc[i].ip,dfc[i].port);
         } else if (strcmp(param,"Username:")==0) {
             strcpy(client_username,strtok(NULL,"\n"));
             //printf(" username:%s\n",client_username);
         } else if (strcmp(param,"Password:")==0) {
             strcpy(client_password,strtok(NULL,"\n"));
             //printf(" password:%s\n",client_password);
         }
    }
    return i;
}

void splitfile (char* filename) {
    int size, i, partsize, remaining;
    char splitfilename[30];
    FILE* fp= fopen(filename,"r");
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp); 
    fseek(fp, 0L, SEEK_SET);
    //printf("file size:%d\n", size);
    partsize= size/totalServers;
    remaining= size%totalServers;
    for (i=1; i<=totalServers; i++ ) {
        sprintf(splitfilename, "%s%s%s%d",".",filename,".",i);

        if (access(splitfilename,F_OK)==0) {
            remove(splitfilename);
        } 

        FILE* splitfile= fopen(splitfilename,"w");
        char ch;
        int j=partsize*i;
        int end=partsize*(i+1);
        if (i==totalServers)
            end=end+remaining;

        while (j++!=end && (ch=fgetc(fp))!=EOF) {
            fputc(ch,splitfile);
        }
        printf("splifile created%s\n",splitfilename );
        fclose(splitfile);
    }
    fclose(fp);  
}

void deletefiles(char* filename) {
    int i;
    char splitfilename[30];
    for (i=1; i<=totalServers; i++ ) {
        sprintf(splitfilename, "%s%s%s%d",".",filename,".",i);

        if (access(splitfilename,F_OK)==0) {
            remove(splitfilename);
        } 
    }
}

char* md5Value (char filename[25],char* hash) {
    int n, val, j, k;
    char md5out[32], out[60];
    char str[40];
    sprintf(str,"md5sum %s", filename); 
    FILE *md5pipe=popen(str, "r");
    fgets(md5out, 32, md5pipe);
    pclose(md5pipe);
    fgets(md5out, 32, md5pipe);
    char *lasttwodigit=&md5out[29];
    val=strtol(lasttwodigit,NULL,16)%4;
    printf("MD5 checksum for file %s is %d\n",filename,val);

    for (j=0,k=1; j < totalServers; j++,k++) {
        hash[(val+j)%4]=k;
    }
    printf("MD5 hash values set :");
    for (j=0; j < totalServers; j++) {
        printf("(%d,%d)", hash[j], hash[(j+1)%totalServers]);
    }
    printf("\n");
    return hash;
}

int topntail(char *str) {
    size_t len = strlen(str);
    int splitpart;
    if (len >= 2) {
        splitpart=str[len-1]-'0';
        memmove(str, str+1, len-2);
        str[len-3] = '\0';
        return splitpart;
    } else {
        printf("Invalid splitfilename\n");
        return 0;
    }
}

int list(int sockfd) {
    int lenRecd;
    char str[26];
    char listname[26];
    int i, n;

    //printf("Files Listed:\n");
    while (1) {
        lenRecd =recv(sockfd,str, sizeof(str)-1, 0);
        //printf("Number recv %d\n", lenRecd);
        str[lenRecd]='\0';
        
        if (strncmp(str,"end",3)==0) {
            return 0;
        }
        if (lenRecd>0) {
           send(sockfd, "ack", 3, 0);
           //printf("%s\n", str);
           n=topntail(str);
           strcpy(listname,&str[0]);
           //printf(".%s.%d\n", listname,n); //don not delete
           for (i=0; i<5; i++) {
            //printf("splitarrayname%sEE\n", splitarray[i]);
                if ((strcmp(splitarray[i],listname)==0)) {
                    break;
                } else if((strcmp(splitarray[i],"")==0)) {
                    //printf("%s inserted into splitarray[%d]\n", listname,i);
                    strcpy(splitarray[i],listname);
                    break;
                }
            }
            //printf("got %dth index of file %s\n", n,listname);
            indexSplit[i][n]=1;   
        } 
    }

}

int get(int sockfd, char* filename) {
    list(sockfd);
    sleep(1);
    int i,j;
    for ( i = 0; i < 5; i++)
    {   //printf("%d--%s--%s\n", i, filename,splitarray[i]);
        if (strcmp(splitarray[i],"")==0) {
            printf("File %s not found in any of DFS%s %s\n", filename,splitarray[i],complete[i]);
            return 0;
        }else if(strcmp(splitarray[i],filename)==0) {
            for(j=1;j<5;j++) {
                if(indexSplit[i][j]==0) {
                    printf("%d part not found for %s\n",j, splitarray[i]);
                    return 0;
                }
            }
            //printf("Matched\n");
            break;
        }
    }

    //printf("File to be processed:%s\n",filename);
    int n, kk;
    char recvBuff[2]="";
   
    for (kk=1; kk<5; kk++) {       
        char file[25];
        sleep(1);
        if ((n=recv(sockfd, file, sizeof(file), 0))>0) {
            if(file[0]=='\0' ||file[0]=='|')
                file[0]='/';
            file[n]='\0';
            //printf("file name%s %d\n", file, n);
            if (access(file,F_OK)==0) {
                printf("splitfile already exists %s\n",file);
                send(sockfd, "end", 3, 0);
                //printf(" send end\n"); //Negative ack for not transmitting redundant files
                continue;
            } else {
                send(sockfd, "ack", 3, 0);
                //printf(" send ack\n");    
                FILE* fd;
                 
                if((fd=fopen(file,"w+"))!=NULL) {
                    sleep(1);
                    //printf("contents:\n");
                        while ( (n = recv(sockfd, recvBuff, sizeof(recvBuff)-1,0)) > 0)
                        {   recvBuff[n] = '\0';
                            if(strncmp(recvBuff,"|",1)==0) 
                                break;
                            else {
                                    fputc(recvBuff[0],fd);
                                    //printf("%s",recvBuff);
                            }
                        } 
                    //printf("Finished receiving\n");
                    printf("\nFile written :%s\n", file);
                    fclose(fd);
                    if ((send(sockfd, "ack", 3, 0)<=0)) {
                        printf("Not send ack\n");
                    } 
                } else {
                    printf("No file opened\n");
                    break;
                }    
            }
        }
    }
}

int put(int sockfd, char* filename) {
    int f, i, count,len, n, fenc;
    char buf[1], ack[4], file[25],str[80], encfile[60];
    strcpy(file,filename);
    //printf("sockfd %d filename %s\n",sockfd, file );
    fflush(NULL);
    //sleep(2);
    if ((f=open(filename,O_RDONLY))!=-1) { 
        if ((n=send(sockfd, file, sizeof(file), 0))<=0) {
            printf("Not sent filename\n");
        } else {
            //printf("send filename AA%sBB %d\n", file, n );
            if ((n=recv(sockfd, ack, sizeof(ack)-1,0))<=0) {
                printf("Not received ack\n");
            } else {
                ack[n]='\0';
                //printf("Ack char %d %sEE \n",n, ack );
            }
        }
        //fflush(NULL);
        strcpy(encfile,file);
        strcat(encfile,"_enc");
        
        sprintf(str,"openssl base64 -in %s -out %s",filename,encfile); 
        FILE *enc= popen(str, "r");
        //printf("encfile: %s for splitfile %s\n", encfile,filename);
        fenc=open(encfile,O_RDONLY);
        while ( (count=read(fenc, buf, sizeof(buf)))>0 ) {
            len=send(sockfd, buf, count,0);
            //printf("sending: %s\n", buf );
        }
        send(sockfd, "|", 1,0);
        //printf("Finished sending\n");
        if ((n=recv(sockfd, ack, sizeof(ack)-1,0))<=0) {
                printf("Not received ack\n");
        } else {
            ack[n]='\0';
            //printf("recvd ack after content transfer, recvd:%s\n ",ack);
        }
    } else {
        printf("File not found\n");
    }
    close(f);
    return 0;
}

void joinfiles(char* filename) {
    int i;
    FILE* fout= fopen(filename,"w+");

    for ( i = 1; i < 5; i++)
    {  
        char splitfilename[30];
        sprintf(splitfilename, "%s%s%s%d",".",filename,".",i);
        FILE* fin;
        char c;
        if((fin=fopen(splitfilename, "r"))!=NULL) {
            while((c=fgetc(fin))!=EOF) {
                fputc(c,fout);
            }
            fclose(fin);
        } else {
            //printf("Split file %s not found (something broken while checking)\n", splitfilename);
        }
    }
    fclose(fout);

}

void *process_thread(void* iter) {
   
    char *option,file[25], ack[4], credentials[50], temp[25];
    int n;
    int index=*(int*)iter;
    //printf("inside thread %p %d\n", (int*)iter,  index);
    
    int cfd=clientid[index];
    send(cfd,opt,sizeof(opt),0);
    if ((n=recv(cfd, ack, sizeof(ack)-1,0)) <=0) {
        printf("Ack not received\nConnection closed from client side...\n");
        //close(cfd);
        exit(0);
    } else {
        ack[n]='\0';
        //printf("ack recvd %s\n",ack );
        if(strcmp(ack,"ack")!=0) {
            printf("Ack not received\nConnection closed from client side...\n");
            exit(0);
        }
    }
    strcpy(temp,opt);
    option=strtok(temp," ");
    //printf("option enteredd: %s\n", option);

    if (strcmp(option,"GET")==0 || strcmp(option,"get")==0) {
        strcpy(file,strtok(NULL," "));
        get(cfd,file);  
    } else if (strcmp(option,"PUT")==0 || strcmp(option,"put")==0) {  
        strcpy(file,strtok(NULL," "));
        char splitfilename[30];
        sprintf(splitfilename, "%s%s%s%d",".",file,".",hash[index-1]);
        //printf("Split file to be send %s for %d thread\n", splitfilename, index);
        put(cfd,splitfilename);    
        int x = (hash[index-1]+1)%5;
        x= (x!=0) ? x: 1;
        sprintf(splitfilename, "%s%s%s%d",".",file,".",x);
        //printf("Split file to be send %s for %d thread\n", splitfilename, index);
        put(cfd,splitfilename);                       
        //printf("Completed file sending for thread %d\n", index);
        deletefiles(file);
    } else if (strcmp(option,"LIST")==0 || strcmp(option,"list")==0) {
        list(cfd);
    } else if (strcmp(option,"0")==0) {
        exit(0);
    } 
}

int main(int argc, char *argv[])
{

    int port, i=1, c, pwdmatch=0, arr[4];
    char *option,file[25], ackpwd[8], credentials[50],temp[30];
    struct sockaddr_in serv_addr; 
    pthread_t ts[totalServers];
  	totalServers=parseDfc();
 
  	for (i=1;i<=totalServers;i++) {

    	if((clientid[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
    	    printf("\n Error : Could not create socket \n");
    	    return 1;
    	} 
		struct hostent *server;
 		server = gethostbyname(dfc[i].ip);
    	memset(&serv_addr, '0', sizeof(serv_addr)); 
    	serv_addr.sin_addr.s_addr= inet_addr(dfc[i].ip);
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_port = htons(dfc[i].port); 
	
	    if (argc!=0 && argv[1]!=NULL && argv[2]!=NULL) {
            strcpy(client_username,argv[1]);
            strcat(client_username,"\t");
            strcpy(client_password,argv[2]);
            printf("credentials passed over runtime: %s %s\n",client_username, client_password );
		}
	    if (connect(clientid[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	       printf("\n Error : Connect Failed for %dth server\n", i);
           clientid[i]=0;
	       //return 1;
    	} 
        
        strcpy(credentials,client_username);
        strcat(credentials,client_password);
        
        //printf("cc:%s le:%d opt:%s\n", credentials, strlen(credentials),opt);
        send(clientid[i],credentials, strlen(credentials),0);
        if ((c=recv(clientid[i], ackpwd, sizeof(ackpwd)-1,0)) <=0) {
            printf("Pwd Ack(correct/wrong) not received\n");
        } else {
            ackpwd[c]='\0';
            if(strcmp(ackpwd,"correct")==0) {
                pwdmatch=1;
                //printf("password matched for client %s\n",client_username);
            }
            else if(strcmp(ackpwd,"wrong")==0) {
                printf("password not matched for client %s\nConnection closed by server\n",client_username );
                break;
            }
            else
                printf("wrong string for password acknowledgement%s\n",ackpwd);
        }
    }

    if (pwdmatch==1)
    while (1) {   
        printf("\nEnter options like below syntax\nLIST\nGET <filename> \nPUT <filename>\n0 to Exit\n");
        fgets (opt, 30, stdin);
    
        if ((strlen(opt)>1) && (opt[strlen(opt)-1] == '\n')) {
            opt[strlen(opt)-1] = '\0';
        } else {
            printf("Invalid option");
            continue;
        }    
        strcpy(temp,opt);
        option=strtok(temp," ");

        if (!(strcmp(option,"GET")==0 || strcmp(option,"get")==0 || \
            strcmp(option,"PUT")==0 || strcmp(option,"put")==0 || \
            strcmp(option,"LIST")==0 || strcmp(option,"list")==0|| \
            strcmp(option,"0")==0)) {
            printf("Invalid option\n");
            continue;
        } else if (strcmp(option,"0")==0) {
            exit(0);
        } else if (strcmp(option,"put")==0 || strcmp(option,"PUT")==0) {
            strcpy(file,strtok(NULL," "));
            md5Value(file,hash);
            splitfile(file);
        } else if (strcmp(option,"get")==0 || strcmp(option,"GET")==0) {
            deletefiles(file);
            strcpy(file,strtok(NULL," "));
            //md5Value(file,hash);
        } else if (strcmp(option,"list")==0 || strcmp(option,"LIST")==0) {
            for ( i = 0; i < 5; i++) {
                strcpy(splitarray[i],"");
                strcpy(complete[i],"");
            }
        }

        for (i=1;i<=totalServers;i++) {
            arr[i-1]=i;
            if (clientid[i]!=0) {
                //printf("Thread created %d\n", i);
                pthread_create(&ts[i],NULL,&process_thread,&arr[i-1]);
                //sleep(2);
            }
        }       

        for (i=1;i<=totalServers;i++) {
            if (clientid[i]!=0) {
                pthread_join(ts[i], NULL);
                //printf("Closing thread %d\n", i);
            }
        }

        if (strcmp(option,"list")==0) {
            int i,j;
            for ( i = 0; i < 5; i++)
            {  
                if(strcmp(splitarray[i],"\0")==0){
                    //printf("File parts not found %s\n", splitarray[i]);
                    break;
                } else {
                    for(j=1;j<5;j++) {
                        if(indexSplit[i][j]==0) {
                            strcpy(complete[i],"[incomplete]");
                            printf("%d part missing for %s\n", j,splitarray[i]);
                            break;
                        } else {
                            strcpy(complete[i],"complete");
                        }
                    }
                    printf("%s %s\n", splitarray[i], complete[i]);
                }

            }
        } else if(strcmp(option,"get")==0) {
            joinfiles(file);
            //printf("File joined\n");
            deletefiles(file);
            for ( i = 0; i < 5; i++) {
                strcpy(splitarray[i],"");
                strcpy(complete[i],"");
            }
        } else if(strcmp(option,"put")==0) {
            printf("File sent\n");
            deletefiles(file);
        }
    }           
            

    return 0;
}
