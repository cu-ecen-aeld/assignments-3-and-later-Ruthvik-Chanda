/*********************************************************************************

Author      : Ruthvik R Chanda
Description : Socket Server Implementation
Reference   : https://beej.us/guide/bgnet/html/
              https://man7.org/linux/man-pages/man3/daemon.3.html
            
*********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>

#define BUFFER_SIZE (1024)
#define PATH "/var/tmp/aesdsocketdata"
#define PORT "9000"

char *client_buff;
int socktfd, clientfd;
int state;


static void sign_handler()
{
	syslog(LOG_INFO, "Exiting and Clearing Buffers\n");
	printf("Clearing Buffers..\n");
	printf("Exiting..\n");
	free(client_buff);
	unlink(PATH);
	close(socktfd);
	close(clientfd);
	exit(EXIT_SUCCESS);	
}

int main(int argc, char *argv[])
{
	int len = 0;

	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
		
	char buff_rx[BUFFER_SIZE];
	
	openlog("AESD - Assignment5 - Socket", 0, LOG_USER);
	
	if( argc > 2)
	{
		printf("Error : Invalid number of arguments\n");
		return -1;
	}

	if ((argc > 1) && (!strcmp("-d", (char*) argv[1])))
	{
	 /* Reference : https://man7.org/linux/man-pages/man3/daemon.3.html */
		state = daemon(0, 0);
		if (state == -1)
		{
			syslog(LOG_DEBUG, "Entering daemon mode failed!");
		}

	}
	
	signal(SIGINT, sign_handler);
	signal(SIGTERM, sign_handler);
	
	struct addrinfo hints;
	struct addrinfo *param;
	
	memset(buff_rx, 0, BUFFER_SIZE);
	memset(&hints, 0, sizeof(hints));
	
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	state = getaddrinfo(NULL, PORT, &hints, &param);
	if (state != 0)
	{
		syslog(LOG_ERR, "Error !! getaddrinfo failed \n");
		exit(EXIT_FAILURE);
	}
	
	socktfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (socktfd == -1)
	{
		syslog(LOG_ERR, "Error !! Socket connection failed \n");
		exit(EXIT_FAILURE);
	}
	
	state = bind(socktfd, param->ai_addr, param->ai_addrlen);
	if (state == -1)
	{
		syslog(LOG_ERR, "Error !! Binding failed : %d\n", state);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(param);
	
	int file_fd;
	file_fd = creat(PATH, 0666);
	if (file_fd == -1)
	{
		syslog(LOG_ERR, "Error !! File creation failed : %d\n", file_fd);
		exit(EXIT_FAILURE);
	}
	
	close(file_fd);
	
	bool p_state = true;
	ssize_t data_rx = 0;
	ssize_t data_wr = 0;
	
	while(1)
	{
		client_buff = (char*) malloc((sizeof(char) *BUFFER_SIZE));
		if (client_buff == NULL)
		{
			syslog(LOG_ERR, "Failed to allocate memory\n");
			exit(EXIT_FAILURE);
		}

		memset(client_buff, 0, BUFFER_SIZE);

		state = listen(socktfd, 10);
		if (state == -1)
		{
			syslog(LOG_ERR, "Error !! Listen failed:%d\n", state);
			exit(EXIT_FAILURE);
		}

		client_addr_len = sizeof(struct sockaddr);

		clientfd = accept(socktfd, (struct sockaddr *) &client_addr, &client_addr_len);
		if (clientfd == -1)
		{
			syslog(LOG_ERR, " Error !! Accepting failed\n");
			exit(EXIT_FAILURE);
		}
		
		p_state = false;
		
		
		while (!p_state)
		{
			
			data_rx = recv(clientfd, buff_rx, BUFFER_SIZE, 0);
			if (data_rx == 0)
			{
				syslog(LOG_INFO,"Reception success\n");
			}
			else if (data_rx < 0)
			{
				syslog(LOG_ERR,"Error !! Recieve failed:%d\n", state);
				exit(EXIT_FAILURE);
			}
			
			int i;
			
			for (i = 0; i < BUFFER_SIZE; i++)
			{
				if (buff_rx[i] == '\n')
				{
					p_state = true;
					i++;
					break;
				}
			}

			len += i;

			client_buff = (char*) realloc(client_buff, (len + 1));
			if (client_buff == NULL)
			{
				syslog(LOG_ERR,"Error !! Realloc failed\n");
				exit(EXIT_FAILURE);
			}
			strncat(client_buff, buff_rx, i);
			memset(buff_rx, 0, BUFFER_SIZE);
		}
		
		
		file_fd = open(PATH, O_APPEND | O_WRONLY);
		if (file_fd == -1)
		{
			syslog(LOG_ERR,"Error !! Failed to open file\n");
			exit(EXIT_FAILURE);
		}
		
		data_wr = write(file_fd, client_buff, strlen(client_buff));
		if (data_wr == -1)
		{
			syslog(LOG_ERR,"Error !! Write failed \n");
			exit(EXIT_FAILURE);
		}
		else if (data_wr != strlen(client_buff))
		{
			syslog(LOG_ERR,"Partial Write\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			syslog(LOG_INFO,"File write success!\n");
		}

		close(file_fd);
		
	
		memset(buff_rx, 0, BUFFER_SIZE);
		file_fd = open(PATH, O_RDONLY);
		if (file_fd == -1)
		{
			syslog(LOG_ERR,"Error !! Open Failed to read\n");
			exit(EXIT_FAILURE);
		}
		
		char char_rd = 0;
		
		for (int i = 0; i < len; i++)
		{
			state = read(file_fd, &char_rd, 1);
			if (state == -1)
			{
				syslog(LOG_ERR,"Error !! Read Failed\n");
				exit(EXIT_FAILURE);
			}

			state = send(clientfd, &char_rd, 1, 0);
			if (state == -1)
			{
				syslog(LOG_ERR, "Error !! Send failed");
				exit(EXIT_FAILURE);
			}
		}
		close(file_fd);
		free(client_buff);
		syslog(LOG_INFO, "Connection closed\n");
	}	
	
	closelog();
	return 0;
}
