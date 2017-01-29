#include	"myfile.h"
#include	<time.h>

int
main(int argc, char **argv)
{
	int					listenfd, connfd;
	struct sockaddr_in	servaddr;
	char				buff[MAXLINE];
	time_t				ticks;
	//char * ip = "127.0.0.1";

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
		exit(0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);	/* daytime server */


	//if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("error in bind");
		exit(0);
	}

	if( listen(listenfd, LISTENQ) <0)
		exit(0);


	for ( ; ; ) {
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		if (connfd<0) {
			perror("connection failure");
			continue;
		}

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        if( write(connfd, buff, strlen(buff)) < 0) {
		    perror("error in writing");
	    }

	close(connfd);
	}
}
