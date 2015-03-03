/*
 *
 proxy.c
 Andrew ID: luyang
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "csapp.h"
#include <pthread.h>
#include <string.h>


#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


#ifdef BIT32
typedef unsigned long long aint;
#else
typedef unsigned long aint;
#endif

static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux i686; rv:1.9.2.3) Gecko/20100423 Firefox/3.6.3\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *accept_0 = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";

int proxy(int fd);
int open_clientfd_safe(char *hostname, int port);
int parse_uri(char *uri, char *hostname, int *port);
int read_request(rio_t *rp, char *buf_request, char *hostname, int *port, char *uri);

void *thread(void *vargp);
void initial();
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
sem_t mutex;
static int set_num, line_num;

static void init();
static int loaddata(char *tag, char *response);
static void storedata(char *response, char *tag);
static void pop_usage(int *array, int key, int len);


struct cache_line
{
   	int valid;
    	char *tag;
    	char *block;
};
struct cache_set
{
    	struct cache_line *line;
    	int *usage;
};
struct cache
{
    	struct cache_set *set;
};
static struct cache c;

int main(int argc,char **argv)
{
        int listenfd, *connfd, port;
    	unsigned clientlen;
    	//struct hostent *hp;
    	struct sockaddr_in clientaddr;
    	//char *haddrp;
    	pthread_t tid;

    	/* Check command line args */
    	if (argc != 2)
    	{
        	fprintf(stderr, "usage: %s <port>\n", argv[0]);
        	exit(1);
    	}

        port = atoi(argv[1]);

    	//ignore sigpipe
    	signal(SIGPIPE, SIG_IGN);
	    //initialization
    	initial();
    	listenfd = Open_listenfd(port);
    	while(1)
    	{
		    connfd = malloc(sizeof(int));
    		clientlen = sizeof(clientaddr);
    		*connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
    		//hp = Gethostbyaddr((const char *) & clientaddr.sin_addr.s_addr,sizeof(clientaddr.sin_addr.s_addr),AF_INET);
    		//haddrp = inet_ntoa(clientaddr.sin_addr);
		    printf("Build new connection! Connfd:%d\n", *connfd);
    		pthread_create(&tid, NULL, thread, connfd);
    	}
}

/* Initialization*/
static void init()
{
    	int i, j;
    	c.set = malloc(sizeof (struct cache_set) * set_num);
    	for (i = 0; i < set_num; i++)
    	{
        	c.set[i].line = malloc(sizeof (struct cache_line) * line_num);
       	 	c.set[i].usage = malloc(sizeof (int) * line_num);
        	for (j = 0; j < line_num; j++)
        	{
            	c.set[i].usage[j] = j;
            	c.set[i].line[j].valid = 0;
		     	c.set[i].line[j].tag = malloc(MAXLINE);
		    	c.set[i].line[j].block = malloc(MAX_OBJECT_SIZE);
        	}
    	}
}
void initial()
{
	    sem_init(&mutex, 0, 1);
	    set_num = 1;
	    line_num = 10;
        init();
}

static void pop_usage(int *array, int key, int len)
{
    	int i, j;
    	for(i = 0; i < len; i++)
    	{
        	if(array[i] == key)
        	break;
    	}
    	for(j = i; j > 0; j--)
    	{
        	array[j] = array[j - 1];
    	}
    	array[0] = key;
}

static int loaddata(char *tag, char *response)
{
    	aint set_index = 0;
    	int i;
    	for(i = 0; i < line_num; i++)
    	{
        	if(c.set[set_index].line[i].valid == 1 && (strcmp(c.set[set_index].line[i].tag, tag) == 0))
        	{
                	pop_usage(c.set[set_index].usage,i,line_num);
               	 	strcpy(response, c.set[set_index].line[i].block);
                	break;
        	}
    	}
     	if(i == line_num)
    	{
        	return 0;
    	}
    	else
    	{
        	return 1;
    	}
}

static void storedata(char *response,char *tag)
{
    	aint set_index = 0;
    	int lru;
    	lru = c.set[set_index].usage[line_num - 1];
    	strcpy(c.set[set_index].line[lru].tag, tag);
    	strcpy(c.set[set_index].line[lru].block, response);
    	if(c.set[set_index].line[lru].valid == 0)
    	{
        	c.set[set_index].line[lru].valid = 1;
    	}
    	pop_usage(c.set[set_index].usage, lru, line_num);

}

/* Thread*/
void *thread(void *vargp)
{
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	free(vargp);
	proxy(connfd);
	close(connfd);
	return NULL;
}

/*open_clientfd function */
int open_clientfd_safe(char *hostname, int port)
{
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;

	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1; /* Check error */
	/* Fill in the server’s IP address and port */
	P(&mutex);
	if ((hp = gethostbyname(hostname)) == NULL)
		return -2; /* Check error */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);
	V(&mutex);
	/* Establish a connection with the server */
	if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
	return -1;
	return clientfd;
}


/*
 * proxy - serve as a proxy to handle one HTTP request/response transaction
 */

int proxy(int connfd)
{
    int port, serverfd, len, response_len;
    char buf_request[MAX_OBJECT_SIZE], buf_response[MAX_OBJECT_SIZE], hostname[MAXLINE], cache_response[MAX_OBJECT_SIZE], uri[MAXLINE];
    rio_t rio_client, rio_server;
    /* reset the buffer */
    memset(buf_request, 0, sizeof(buf_request));
    memset(buf_response, 0, sizeof(buf_response));
    memset(hostname, 0, sizeof(hostname));
    memset(cache_response, 0, sizeof(cache_response));
    memset(uri, 0, sizeof(uri));
    /* Read request line and headers */
    rio_readinitb(&rio_client, connfd);
    printf("read_request in!\n");

    if(read_request(&rio_client, buf_request, hostname, &port, uri) < 0)
        return -1;
    printf("read_request out!\n");
    printf("uri:%s\thostname:%s\tport:%d\n", uri, hostname, port);
    printf("buf_request: %s\n", buf_request);
    if((loaddata(uri, cache_response)) == 1)
	{
        printf("Cache hit!\n");
        if(rio_writen(connfd, cache_response, sizeof(cache_response)) < 0)
        {
                fprintf(stderr, "cache response error\n");
                return -1;
        }
        /*reset the buffer*/
        memset(cache_response, 0, sizeof(cache_response));
	}
    else
	{
		printf("Cache miss!\n");
		/*send request to real server*/
		if((serverfd = open_clientfd_safe(hostname, port)) < 0)
        {
        		fprintf(stderr, "open serverfd error\n");
        		return -1;
        }
		rio_readinitb(&rio_server, serverfd);

        if(rio_writen(serverfd, buf_request, strlen(buf_request)) < 0)
    	{
        		printf("buf_request error:\n%s\n", buf_request);
			    fprintf(stderr, "rio_writen send request error\n");
      			close(serverfd);
       		    return -1;
    	}

		/*receive from server and send back to the client */
		memset(cache_response, 0, sizeof(cache_response));
		response_len = 0;
		while((len = rio_readnb(&rio_server, buf_response, sizeof(buf_response))) > 0)
        {
      		printf("hostname:%s\tport:%d\nbuf_response:%s\n", hostname, port, buf_response);
			strcat(cache_response, buf_response);
			response_len += len;
        	if(rio_writen(connfd, buf_response, len) < 0)
            {
        			printf("buf_response error:%s\nlen:%d\n%s\n", strerror(errno), len, buf_response);
                    fprintf(stderr, "rio_writen send response error\n");
				    close(serverfd);
				    return -1;
            }
			/* reset the buffer*/
			memset(buf_response, 0, sizeof(buf_response));
    		}
		    if(response_len <= MAX_OBJECT_SIZE)
			storedata(uri, cache_response);
			close(serverfd);
	}

    return 0;
}

/*
 * read_request - read and parse HTTP request headers
 */
int read_request(rio_t *rp, char *buf_request, char *hostname, int *port, char *uri)
{
    char buf[MAXLINE], method[MAXLINE];
	memset(buf, 0, sizeof(buf));
	memset(method, 0, sizeof(method));
	/* request line */
	if(rio_readlineb(rp, buf, MAXLINE) <= 0)
    return -1;
	printf("buf: %s\n", buf);
    sscanf(buf, "%s %s", method, uri);

	/* get hostname and port information*/
	parse_uri(uri, hostname, port);
    /* fill in request for real server */
    sprintf(buf_request, "%s %s HTTP/1.0\r\n", method, uri);
	printf("buf_request: %s\n", buf_request);
    if(rio_readlineb(rp, buf, MAXLINE) < 0)
	{
		printf("rio_readlineb fail!\n");
        return -1;
    }
	while(strcmp(buf, "\r\n"))
	{
		printf("buf: %s\n", buf);
		if(strcmp(buf, "\n") == 0)
		{
			printf("bufn \n");
		}
		if(strcmp(buf, "\r") == 0)
		{
			printf("bufr \n");
		}
        if(strstr(buf, "Host"))
        {
            strcat(buf_request, "Host: ");
            strcat(buf_request, hostname);
            strcat(buf_request, "\r\n");
        }
        else if(strstr(buf, "Accept:"))
            strcat(buf_request, accept_0);
        else if(strstr(buf, "Proxy-Connection:"))
            strcat(buf_request, "Proxy-Connection: close\r\n");
        else if(strstr(buf, "User-Agent:"))
            strcat(buf_request, user_agent);
        else if(strstr(buf, "Accept-Encoding:"))
            strcat(buf_request, accept_encoding);
        else if(strstr(buf, "Connection:"))
            strcat(buf_request, "Connection: close\r\n");
        else if(!strstr(buf, "Cookie:"))
        /*add addtional header except above headers*/
            strcat(buf_request, buf);
		    memset(buf, 0, sizeof(buf));
        if(rio_readlineb(rp, buf, MAXLINE) < 0)
        {
            fprintf(stderr, "rio_readlineb read request error\n");
            return -1;
        }
    }

  	strcat(buf_request, "\r\n");
	return 0;
}

/*
 * parse_uri - parse URI into hostname and port args
 */

int parse_uri(char *uri, char *hostname, int *port)
{
    char *ps, *pe, *phost;
    char *uricp = malloc(strlen(uri) + 1);
    strncpy(uricp, uri, strlen(uri));

    /* start pointer */
    if((ps = strstr(uricp, "http://")) == NULL)
        return -1;
    ps += strlen("http://");
    /* end pointer */
    if((pe = strstr(ps, "/")) == NULL)
        return -1;
    *pe = '\0';
	/*if hostname contains port*/
	phost = strsep(&ps, ":");
    if(ps == NULL)
        *port = 80;
    else
        *port = atoi(ps);
    	strncpy(hostname, phost, strlen(phost));
    	return 0;
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
	memset(buf, 0, sizeof(buf));
	memset(buf, 0, sizeof(body));
    /* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
   	sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
   	rio_writen(fd, body, strlen(body));
}
