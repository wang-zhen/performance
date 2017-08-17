#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>   
#include <sys/wait.h>      
#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <sys/poll.h>

#define USERMAX  5
#define FD_MAX  65535
#define BUFFER_SIZE  64

typedef struct {

	struct sockaddr_in address;
	char *write_buffer;
	char buffer[BUFFER_SIZE];
}client_data;

static struct pollfd fds[USERMAX+1];
static int user_count = 0;

int setnonblocking (int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option );
	return old_option;
}

/**
 ** Main entry point...
 ***/
int main(int argc, char *argv[])
{
	char *ipaddr = NULL;
	client_data *users = NULL;
	int i, j, ret, port, sockfd;
	int go_on = 1;
	struct sockaddr_in server_addr;

	if(argc <= 2){
		fprintf(stdout,"%s <ipaddr> <port>\n",argv[0]);
		exit(-1);
	}
	
	ipaddr = argv[1];
	port = atoi(argv[2]);

	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, ipaddr, &server_addr.sin_addr);
	server_addr.sin_port=htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket < 0){
		perror("create socket error");
		exit(-1);
	}

	if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) == -1){
		perror("bind error");
		exit(-1);
	}	

	if(listen(sockfd,5) == -1){
		perror("listen error");
		exit(-1);
	}

	if( !(users = malloc(FD_MAX * sizeof(client_data))) ){
		perror("malloc error");
		exit(-1);
	}

	for(i=0;i<USERMAX+1;i++){
		fds[i].fd = -1;
		fds[i].events = 0;
	}

	fds[0].fd = sockfd;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;

	while(go_on){
		ret = poll(fds, USERMAX+1, -1);
		if(ret < 0){
			fprintf(stderr,"pool error!\n");
			continue;
		}

		fprintf(stdout,"poll...\n");
		for(i=0; i<USERMAX + 1; ++i){
			if((fds[i].revents & POLLIN) && (fds[i].fd == sockfd)){
				/*listen socket*/	
				struct sockaddr_in client_addr;
				socklen_t caddr_len = sizeof(client_addr);
				int conn = accept(sockfd, (struct sockaddr*)&client_addr, &caddr_len);
				if(conn < 0){
					perror("connect error");
					continue;
				}
			
				if(user_count >= USERMAX){
					char *info = "too many users!";
					fprintf(stderr, "%s\n",info);
					send(conn, info, strlen(info), 0);
					close(conn);
					continue;
				}
				
				setnonblocking(conn);

				user_count++;
				users[conn].address = client_addr;

				fds[user_count].fd = conn;
				fds[user_count].events = POLLIN | POLLRDHUP | POLLERR;
				fds[user_count].revents = 0;
				fprintf(stdout,"new user comeing!\n");	
			}else if(fds[i].revents & POLLERR){
				/*poll err*/
				fprintf(stderr, "get an error from %d\n",fds[i].fd);
				char errors[100];
				memset(errors, '\0', 100);
				socklen_t length = sizeof(socklen_t);

				if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length)){
					fprintf(stderr, "get an error from %d\n",fds[i].fd);
				}
				continue;
			}else if(fds[i].revents & POLLRDHUP){
				/*disconnect*/
				fprintf(stderr, "disconnect: %d\n",fds[i].fd);
				users[fds[i].fd] = users[fds[user_count].fd];

				close(fds[i].fd);

				fds[i] = fds[user_count];

				fds[user_count].fd = -1;
				fds[user_count].events = 0;
				fds[user_count].revents = 0;

				i--;
				user_count--;
			}else if(fds[i].revents & POLLIN){
				/*read*/
				fprintf(stderr, "read from %d\n",fds[i].fd);
				int connfd = fds[i].fd;
				memset(users[connfd].buffer, '\0', BUFFER_SIZE);

				ret = recv(connfd, users[connfd].buffer, BUFFER_SIZE-1, 0);
				if(ret < 0){
					
				}else if(ret == 0){

				}else{
					for(j=1; j <= user_count; j++){
						if(fds[j].fd == connfd){
							continue;
						}

						fds[j].events |= ~POLLIN;
						fds[j].events |= POLLOUT;
						users[fds[j].fd].write_buffer = users[connfd].buffer;
					}
				}
				fprintf(stdout, "recv data :%s\n",users[connfd].buffer);
			}else if(fds[i].revents & POLLOUT){
				/*write*/
				fprintf(stderr, "write to %d\n",fds[i].fd);
				int connfd = fds[i].fd;
				if(!users[connfd].write_buffer){
					fprintf(stderr,"write error!\n");
					continue;
				}
				
				ret = send(connfd, users[connfd].write_buffer, strlen(users[connfd].write_buffer), 0);
				if(ret < 0){
					perror("send error\n");
					continue;
				}
				
				users[connfd].write_buffer = NULL;
				//fds[i].events |= ~POLLOUT;
				//fds[i].events |= POLLIN;
				fds[i].events = POLLIN | POLLRDHUP | POLLERR;
				fds[i].revents = 0;
				fprintf(stderr, "just for test\n");
			}
		}
	}

	free(users);
	close(sockfd);
	return 0 ;
}
