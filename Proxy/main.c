#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>

int multiplier  = 3;
int server_size = 27*(1 << 13)


void err_sys(const char* error);

typedef struct client
{
	int fds[4];
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

	client_structs[0].fds[0] = -1;
	client_structs[0].fds[1] = -1;
	client_structs[0].fds[2] = input_fd;
	client_structs[0].fds[3] = -1;

	client_structs[n-1].fds[0] = -1;
	client_structs[n-1].fds[1] =  1;
	client_structs[n-1].fds[2] = -1;
	client_structs[n-1].fds[3] = -1;

	for(int i = 1; i < n-1; i++)
	{
		pipe(client_structs[i].fds[0]);
		pipe(client_structs[i].fds[2]);
		fcntl(client_structs[i].fds[3], F_SETFL, O_WRONLY|O_NONBLOCK);
	}

	for (int i = 0; i < N; i++)
	{
		if(fork == 0)
		{
			for (int j = 0; j < N; j++)
			{
				if(i != j)
				{
					close(client_structs[j].fds[0]);
					close(client_structs[j].fds[1]);
					close(client_structs[j].fds[2]);
					close(client_structs[j].fds[3]);
				}
				else
				{
					close(client_structs[j].fds[0]);
					close(client_structs[j].fds[3]);
				}
			}
		}
	}

}

void err_sys(const char* error)
{
	perror(error);
}
	exit(1);
