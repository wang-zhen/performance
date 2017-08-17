#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>     /* select */
#include <sys/wait.h>       /* wait */
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mount.h>      /* For BLKGETSIZE */
#include <signal.h>     /* sigaction */
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

#include <glib.h>

#define BUFFER_SIZE  64

char read_buf[BUFFER_SIZE];
static struct pollfd fds[3];

/**
 ** Main entry point...
 ***/
int main(int argc, char *argv[])
{
	char *ipaddr = NULL;
	int i, j, ret, port, sockfd;
	int piped[2];
	struct sockaddr_in server_addr;
	int go_on = 1;

	if(argc  <= 2){
		fprintf(stdout,"%s <ipaddr> <port>\n",argv[0]);
		exit(-1);
	}
	
	ipaddr = argv[1];
	port = atoi(argv[2]);

	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, ipaddr, &server_addr.sin_addr);
	server_addr.sin_port=htons(port);

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(socket < 0){
		perror("create socket error");
		exit(-1);
	}

	if(connect(sockfd,(struct sockaddr *)(&server_addr), \
		sizeof(struct sockaddr)) == -1){
		perror("connect error");
		exit(-1);
	}	

	for(i=0;i<2;i++){
		fds[i].fd = -1;
		fds[i].events = 0;
	}

	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;
	fds[1].revents = 0;

	if(pipe(piped) < 0){
		perror("connect error");
		exit(-1);
	}

	while(go_on){
		ret = poll(fds, 2, -1);
		if(ret < 0){
			fprintf(stderr,"pool error!\n");
			go_on = 0;
			continue;
		}

		if(fds[1].revents & POLLRDHUP){
			/*disconnect*/
			fprintf(stdout, "disconnect!\n");
			close(fds[1].fd);
			fds[1].fd = -1;
			fds[1].events = 0;
			go_on = 0;
			continue;
		}else if((fds[1].revents & POLLIN)){
			/*listen socket*/	
			fprintf(stdout, "read!\n");
			memset(read_buf, 0, BUFFER_SIZE);

			recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
			fprintf(stdout,">>recv: %s\n",read_buf);

		}
		if(fds[0].revents & POLLIN){
			/*read from stdin*/
			ret = splice(0, NULL, piped[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			ret = splice(piped[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
		}
	}

	close(sockfd);
	return 0 ;
}
