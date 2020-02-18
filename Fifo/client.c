#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define     BUFFSIZE	2048
const char* ADDRFIFO = "/tmp/addrfifo";
char pidname[12];

void err_sys(const char* error)
{
	perror(error);
	exit(1);
}

int main()
{
	int  n;
	char buf[BUFFSIZE];
	int  pid_int = getpid();

	
	sprintf(pidname, "%d", pid_int);

	if((mkfifo(ADDRFIFO, 0644) < 0) && (errno != EEXIST))
		err_sys("MKFIFO ERROR");

	int addr_fd = open(ADDRFIFO, O_WRONLY);
	
	if((addr_fd < 0) && (errno != EEXIST))
		err_sys("OPEN FIFO ERROR");
	
	if((mkfifo(pidname, 0644) < 0) && (errno != EEXIST))
		err_sys("MKFIFO ERROR");
	
	int main_fd = open(pidname, O_RDONLY | O_NONBLOCK);
	
	if((main_fd < 0) && (errno != EEXIST))
	{

		err_sys("OPEN FIFO ERROR");
	}

	if (write(addr_fd, &pid_int, sizeof(int)) != sizeof(int))
	{
			err_sys("WRITE ERROR");
	}
	close(addr_fd);

	int succesful_rd = 0;

/////////////////////begin///////////////////////////////
	for(int i = 0; i < 15; i++)
	{
		if((n = read(main_fd, buf, BUFFSIZE)) == 0)
		{
			sleep(1);
		}
		if(n < 0)
		{				
			err_sys("FIFO READ ERROR");
		}
		if(n > 0)
		{
			if (write(STDOUT_FILENO, buf, n) != n)
			{

				err_sys("WRITE ERROR");
			}	
			succesful_rd = 1;
			
			break;

		}
	}

	if(succesful_rd == 0)
	{

		err_sys("SERVER TIMEOUT");
	}

	else	
	{
		while ((n = read(main_fd, buf, BUFFSIZE)) > 0)
			if (write(STDOUT_FILENO, buf, n) != n)
			{

				err_sys("WRITE ERROR");
			}

		if (n < 0)
		{
			err_sys("READ ERRRO");
		}

	}

////////////////////////end///////////////////////////////
	remove(pidname);
	exit(0);
}
