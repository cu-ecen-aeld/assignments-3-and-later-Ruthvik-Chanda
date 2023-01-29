#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
	openlog(NULL, 0, LOG_USER);
	
	if ( argc != 3)
	{
		printf(" ERROR !! Invalid number of arguments\n\r");
		printf("Error !! Enter the correct arguments as <file_name> <string_to_enter>\n\r");
		syslog(LOG_ERR, "Invalid number of arguments: %d", argc);
		closelog();
		return 1;
	}
	
	int fd = open( argv[1], O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH | S_IWOTH );
	
	if (fd == -1)
	{
		printf(" ERROR !! File not found\n\r");
		syslog(LOG_ERR, "ERROR !! File not found: %d", errno);
		closelog();
		return 1;
	}
	
	
	int write_byte_out = write(fd, argv[2], strlen(argv[2]));
	
	if(write_byte_out == -1)
	{
		printf("ERROR !! Write failed\n\r");
		syslog(LOG_ERR, "ERROR !! Write failed");
		closelog();
		return 1;
	}
	
	else if((write_byte_out != strlen(argv[2])))
	{
		printf("ERROR !! Partial Write\n\r");
		syslog(LOG_ERR, "ERROR !! Partial Write of %s", argv[2]);
		closelog();
		return 1;
	}
	
	close(fd);
	
	return 0;	
}
