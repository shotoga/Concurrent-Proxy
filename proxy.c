/*
 * proxy.c - A Simple Sequential Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 2
 * Student Name:Shota Ogawa
 * 
 */

#include <sys/socket.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
/*
 * Function prototypes
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

void err_exit(char reason[])
{
  perror(reason);
  exit(2);
}

	
/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
  /* Check arguments */
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    exit(0);
  }
  //socket for server---------------------------------
  int sock=socket(AF_INET,SOCK_STREAM,0);
  if(sock<0)
    err_exit("socket error");
  else
    printf("socket created\n");
  //initialize server information
  struct sockaddr_in servaddr;
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  if(argc==2)
    {
      //using input port
      int portNum=atoi(argv[1]);
      servaddr.sin_port=htons(portNum);
      printf("initializing port %d\n",portNum);
    }
  else
    err_exit("not a valid port");
  //using any ip address
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  //-----------------------------------------------

  //bind to a socket with defined specifications
  int bind_err=bind(sock,(struct sockaddr*) &servaddr,sizeof(servaddr));
  if(bind_err<0)
    err_exit("bind error");
  else
    printf("bind success\n");
    
  //listen for incoming connections to socket
  int listen_err=listen(sock,5);
  if(listen_err<0)
    err_exit("listen error");
  else
    printf("listen success\n");
	
  //accept any connection
  int cfd=accept(sock,NULL,0);
  if(cfd<0)
    err_exit("accept error");
  else
    printf("accept success\n");

  //read client's request
  char reqBuf[10000];
  int reqLen=read(cfd,reqBuf,sizeof(reqBuf));
  reqBuf[reqLen]=0;
  if(reqLen<0)
    err_exit("read request error");
  else
    printf("read request success\n");

  //parsing information-------------------------------------------------
  //if first 3 elements doesn't say get, print an error
  if(strncmp(reqBuf,"GET",3)!=0)
    {
      fprintf(stderr,"not a get request\n");
      exit(2);
    }
  char *s=reqBuf;
  //printf("%s\n",reqBuf);
  //should be GET [website]/destination/ HTTP/1.0 OR 1.1
  //we must parse this information and store into log file
  //get GET message
  char *getReqm=strsep(&s,"\r\n");
  char getMessage[100];
  strcpy(getMessage,getReqm);
  printf("get message:%s\n",getMessage);
  strsep(&s," ");
  //get host name
  char *host=strsep(&s,"\r\n");
  printf("host:%s\n",host);
  s=reqBuf;
  //get URL
  strsep(&s," ");
  char *URL=strsep(&s," ");
  printf("URL:%s\n",URL);
  int i=0;
  int j=0;
  int k=0;
  //i couldn't figure out how to extract just the search so i used brute force and did it manually
  char dest[100];
  //count size of host and url(strsep doesn't end the string)
  while(host[i]!=NULL)
    i++;
  while(URL[j]!=NULL)
    j++;
  while(getReqm[k]!=NULL)
    k++;
  k++;
  while(getReqm[k]!=NULL)
    k++;
  k++;
  while(getReqm[k]!=NULL)
    k++;
  int getSize=k;
  printf("get message size:%d\n",getSize);
  int hostSize=i;
  int URLSize=j;
  //printf("\n\n%d,%d\n\n",hostSize,URLSize);
  i=0;
  j=0;
  k=0;
  //couldn't figure out how to compare the strings with strncmp so again i used brute force
  for(i=0;i<URLSize;i++)
    {
      //printf("%c,%c\n",host[j],URL[i]);
      if(host[j]==URL[i])
	j++;
      else
	j=0;
      if(j==hostSize)
	break;
    }
  i++;
  //printf("%d\n",i);
  j=0;
  //assign the directory to dest
  for(i=i;i<URLSize;i++)
    {
      dest[j]=URL[i];
      j++;
    }
  printf("destination:%s\n",dest);
  i=0;
  char *getReq=getReqm;
  //add get size (3)
  while(getReq[i]!=NULL)
    i++;
  //add null(1)
  i++;
  //add url size
  while(getReq[i]!=NULL)
    i++;
  //add NULL and HTTP/1.(8)
  i+=8;
  int HTTP=0;
  //printf("%c",getReq[i]);
  if(getReq[i]=='1')
    HTTP=1;
  printf("http type:HTTP/1.%d\n",HTTP);
  //----------------------------------------------------------------------
  //help from binaryTides.com to find IP(couldn't find it in the web request)-----
  struct hostent *hostaddr;
  struct in_addr **addr_list;
  hostaddr=gethostbyname(host);
  if(hostaddr==NULL)
    err_exit("host address error");
  else
    printf("host address retreived\n");
  addr_list=(struct in_addr **)hostaddr->h_addr_list;
  char ip[50];
  for(i=0;addr_list[i]!=NULL;i++)
    {
      strcpy(ip,inet_ntoa(*addr_list[i]));
    }
  servaddr.sin_addr.s_addr=inet_addr(ip);
  //--------------------------------------------------------------------
  printf("IP:%s\n\n",ip);

  //logging
  char logstring[10000];
  format_log_entry(logstring,&servaddr,URL,0);
  if(logstring==NULL)
    err_exit("logging error");
  printf("log:%s\n",logstring);
  //write log to file
  FILE *fp=fopen("log.txt","a+");
  if(fp==NULL)
    err_exit("opening log file error");
  else
    printf("writing to file\n");
  fprintf(fp,logstring);

  //client here
  //socketClient
  int sockCli=socket(AF_INET,SOCK_STREAM,0);
  if(sock<0)
    err_exit("socketCli error");
  else
    printf("socket for client created\n");
  //initialize server information
  struct sockaddr_in cliaddr;
  bzero(&cliaddr,sizeof(cliaddr));
  cliaddr.sin_family=AF_INET;
  //want to connect to internet (80 is default port)
  cliaddr.sin_port=htons(80);
  //use the ip address we extracted
  cliaddr.sin_addr.s_addr=servaddr.sin_addr.s_addr;
    
  //connect to the internet
  int connect_err=connect(sockCli,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
  if(connect_err<0)
    err_exit("connecting to web error");
  else
    printf("connection to web success\n");

  //write to web
  //retreiving get message b/c it was a separated string
  char getMsg[100];
  strcat(getMsg,"GET ");
  strcat(getMsg,dest);
  strcat(getMsg," HTTP/1.");
  /*if(HTTP==1)
    strcat(getMsg,"0\r\n\r\n");
    else if(HTTP==0)*/
  //for some reason when the get message was 1.1, it only worked when i input 1.0
  strcat(getMsg,"0\r\n\r\n");
  else
    err_exit("HTTP not 1 or 0 error");
  printf("getMsg:%s\n",getMsg);
  int write_err=write(sockCli,getMsg,sizeof(getMsg));
  if(write_err<0)
    err_exit("writing to web error");
  else
    printf("%s\n",getMsg);

  //read
  //large buffer for large sites
  char webResp[1000000];
  int read_err=read(sockCli,webResp,sizeof(webResp));
  if(read_err<0)
    err_exit("receiving error");
  else
    //write response to client
    {
      write_err=write(cfd,webResp,sizeof(webResp));
      if(write_err<0)
	err_exit("write to client error");
      else
	printf("write to client success\n");
	
    }
  //close connections
  int close_err=close(cfd);
  if(close_err<0)
    err_exit("closing server error");
  else
    printf("closed\n");
  close_err=close(connect_err);
  if(close_err<0)
    err_exit("closing client error");
  else
    printf("closed\n");
  return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
  time_t now;
  char time_str[1000];
  unsigned long host;
  unsigned char a, b, c, d;

  /* Get a formatted time string */
  now = time(NULL);
  strftime(time_str, 1000, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

  /* 
   * Convert the IP address in network byte order to dotted decimal
   * form. Note that we could have used inet_ntoa, but chose not to
   * because inet_ntoa is a Class 3 thread unsafe function that
   * returns a pointer to a static variable (Ch 13, CS:APP).
   */
  host = ntohl(sockaddr->sin_addr.s_addr);
  a = host >> 24;
  b = (host >> 16) & 0xff;
  c = (host >> 8) & 0xff;
  d = host & 0xff;


  /* Return the formatted log entry string */
  sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


