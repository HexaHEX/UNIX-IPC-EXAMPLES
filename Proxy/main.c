#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>

#define BUFFSIZE 27*(1 << 10)


void err_sys(const char* error);

typedef struct client
{
	int fds1[2];
	int fds2[2];
	char* data;
	char* freedata;
	int capacity;
	int size;

} client;


int main(int argc, char* argv[])
{
	int n = 2;
	int input_fd = 0;

	client* client_structs = NULL;

	if(argc != 3)
	{
		err_sys("INCORRECT INPUT");
	}

	n += atoi(argv[2]);
	client_structs = (client*)calloc(n, sizeof(client));

	input_fd = open(argv[1], O_RDONLY);
	client_structs[0].fds1[0] = -1;
	client_structs[0].fds1[1] = -1;
	client_structs[0].fds2[0] = input_fd;
	client_structs[0].fds2[1] = -1;

	client_structs[n-1].fds1[0] = -1;
	client_structs[n-1].fds1[1] =  1;
	client_structs[n-1].fds2[0] = -1;
	client_structs[n-1].fds2[1] = -1;

	for(int i = 1; i < n-1; i++)
	{

		pipe(client_structs[i].fds1);
		pipe(client_structs[i].fds2);
		fcntl(client_structs[i].fds1[1], F_SETFL, O_WRONLY|O_NONBLOCK);
	}
	for (int i = 1; i < n-1; i++)
	{
		if(fork() == 0)
		{
			for (int j = 1; j < n-1; j++)
			{
				if(i != j)
				{
					close(client_structs[j].fds1[0]);
					close(client_structs[j].fds1[1]);
					close(client_structs[j].fds2[0]);
					close(client_structs[j].fds2[1]);
				}
				else
				{
					close(client_structs[j].fds1[1]);
					close(client_structs[j].fds2[0]);
				}
			}
			int in = client_structs[i].fds1[0];
			int out  = client_structs[i].fds2[1];
			free(client_structs);
			char data[BUFFSIZE] = "";
			int ret = 1;
			while(ret)
			{
				ret = read(in, data, BUFFSIZE);
				if(ret < 0)
					err_sys("READ ERROR");
				if(write(out, data, ret) < ret)
					err_sys("WRITE ERROR");			}
			close(in);
			close(out);
			exit(0);
		}
	}
	for(int i = 1; i < n-1; i++)
	{
		close(client_structs[i].fds1[0]);
		close(client_structs[i].fds2[1]);
	}

	int mul = 1;
	int maxfd = -1;

	for(int i = n-1; i > 0; i--)
	{
		client_structs[i].capacity = mul * 1 << 10;
		client_structs[i].freedata = (char*)calloc(client_structs[i].capacity, sizeof(char));
		client_structs[i].data = client_structs[i].freedata;
		client_structs[i].size = 0;
		if(mul <= 27)
		{
			mul *= 3;
		}
	}

	while (1)
	{
		fd_set readable;
		fd_set writeable;
		int fds = 0;

		FD_ZERO(&readable);
		FD_ZERO(&writeable);

		for(int i = 0; i < n; i++)
		{
			if(client_structs[i].fds1[1] > maxfd)
				maxfd = client_structs[i].fds1[1];

			if(client_structs[i].fds2[0] > maxfd)
				maxfd = client_structs[i].fds2[0];

			if((client_structs[i].fds2[0] != -1) && (client_structs[i+1].size == 0))
			{
				FD_SET(client_structs[i].fds2[0], &readable);
				fds++;
			}

			if((client_structs[i].fds1[1] != -1) && (client_structs[i].size != 0))
			{
				FD_SET(client_structs[i].fds1[1], &writeable);
				fds++;
			}
		}

		if(fds == 0)
			break;
		int selectret = select(maxfd + 1, &readable, &writeable, NULL, NULL);

		if(selectret == 0)
			break;
		if(selectret == -1)
			err_sys("SELECT ERROR");

		for(int i = 0; i < n; i++)
		{
			if(FD_ISSET(client_structs[i].fds2[0], &readable))
			{
				int readret = read(client_structs[i].fds2[0], client_structs[i+1].data, client_structs[i+1].capacity);
				if(readret == -1)
					err_sys("READ ERROR");
				client_structs[i+1].size += readret;
				if(readret == 0)
				{
					close(client_structs[i].fds2[0]);
					close(client_structs[i+1].fds1[1]);
					client_structs[i].fds2[0] = client_structs[i+1].fds1[1] = -1;
				}
			}

			if(FD_ISSET(client_structs[i].fds1[1], &writeable))
			{
				int writeret = write(client_structs[i].fds1[1], client_structs[i+1].freedata, client_structs[i+1].size);

				if(writeret == -1)
					err_sys("READ ERROR");
				if(writeret < client_structs[i].size)
					client_structs[i].freedata += writeret;
				else
					client_structs[i].freedata = client_structs[i].data;
				client_structs[i].size -= writeret;
			}
		}
	}
	for(int i = 1; i < n; i++)
		free(client_structs[i].data);

	free(client_structs);
	exit(0);
}

void err_sys(const char* error)
{
	perror(error);
	exit(1);
}
