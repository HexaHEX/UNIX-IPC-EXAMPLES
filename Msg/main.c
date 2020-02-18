#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/msg.h>

void err_sys(const char* error)
{
    perror(error);
    exit(1);
}

struct msgstr
{
    long type;
};

int main(int argc, char* argv[])
{
    setbuf(stdout, NULL);
    if(argc != 2)
    {
        errno = EINVAL;
        err_sys("INCORRECT INPUT");
    }

    long n = atoi(argv[1]);

    if(n <= 0)
    {
        errno = EINVAL;
        err_sys("INCORRECT INPUT");
    }

    int que_id = msgget(IPC_PRIVATE, 0600);
    if(que_id < 0)
        err_sys("MSGGET ERROR");

	int my_id  = 0;

	for(int i = 0; i < n; i++)
	{
        int pid = fork();
        if(pid < 0)
	{
            err_sys("FORK ERROR");
	}
        if(!pid)
		{
			my_id = i + 1;
			break;
		}

	}

	if(my_id == 0) {

        	struct msgstr msg = {1};
        	if(msgrcv(que_id, &msg, 0, n+1, 0) < 0)
        		err_sys("MSGRCV ERROR");
		exit(0);

	}
	
    if(my_id == n)
	{
        struct msgstr msg = {1};
		if(msgsnd(que_id, &msg, 0, 0))
		    err_sys("MSGSND ERROR");

        if(msgrcv(que_id, &msg, 0, n, 0))
            err_sys("MSGRCV ERROR");

        msg.type = my_id + 1;

	printf("%d ", n);

        if(msgsnd(que_id, &msg, 0, 0))
            err_sys("MSGSND ERROR");

    }
	else
	{
        struct msgstr msg = {0};
        if(msgrcv(que_id, &msg, 0, my_id, 0))
            err_sys("MSGRCV ERROR");

        msg.type = my_id + 1;
	printf("%d ", my_id);
	
	if(msgsnd(que_id, &msg, 0, 0))
            err_sys("MSGSND ERROR");



    }
	exit(0);
}

