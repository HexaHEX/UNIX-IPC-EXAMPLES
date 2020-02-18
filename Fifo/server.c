#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define     BUFFSIZE	2048
const char* ADDRFIFO = "/tmp/addrfifo";

void err_sys(const char* error)
{
	perror(error);
	exit(1);
}

int main(int argc, char* argv[])
{
	int  n;
	char buf[BUFFSIZE];
	
	int  pid_int;
	char mainfifo[12];

	if(argc != 2)
		err_sys("INCORRECT INPUT");
	
	int input_fd = open(argv[1], O_RDONLY);

	if((input_fd < 0) && (errno != EEXIST))
		err_sys("WRONG INPUT FILE");

	if((mkfifo(ADDRFIFO, 0644) < 0) && (errno != EEXIST))
		err_sys("MKFIFO ERROR");
/////////////////////begin///////////////////////////////
	int addr_fd = open(ADDRFIFO, O_RDWR);

	if((addr_fd < 0) && (errno != EEXIST))
		err_sys("OPEN FIFO ERROR");

	
	n = read(addr_fd, &pid_int, sizeof(int));
	if(n < 0)
		err_sys("READ ERROR");
	close(addr_fd);

	sprintf(mainfifo, "%d", pid_int);	

	if((mkfifo(mainfifo, 0644) < 0) && (errno != EEXIST))
	{

		err_sys("MKFIFO ERROR");		
	}

	int helper = open(mainfifo, O_RDONLY | O_NONBLOCK);
	int main_fd = open(mainfifo, O_WRONLY);

	close(helper);	

	if((main_fd < 0) && (errno != EEXIST))
	{

		err_sys("OPEN FIFO ERROR");
	}
	
	n = read(input_fd, buf, BUFFSIZE);
	if (write(main_fd, buf, n) != n)
		{	

			err_sys("WRITE ERROR");
		}
	
	
	
	while ((n = read(input_fd, buf, BUFFSIZE)) > 0)
		if (write(main_fd, buf, n) != n)
		{	

			err_sys("WRITE ERROR");
		}

	if (n < 0)
	{

		err_sys("READ ERRRO");
	}
	/////////////////////end//////////////////////////////////
	remove(mainfifo);
	exit(0);
}
