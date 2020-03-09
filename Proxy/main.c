#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHILDBUFSIZE 4096
#define MAXCAPACITY  128

typedef struct conn {
    int     fds[2];
    char*   buf;
    int     buf_capacity;
    int     size;
    int     offset;
    int     dead;
} conn_t;

void err_sys(const char* error);
int connect(int n, conn_t* connections, int* child_fds, int first_child_read_fd);

int main(int argc, char* argv[])
{
    if(argc != 3)
      err_sys("INCORRECT INPUT. USE <input path><children number>");

    int n = atoi(argv[2]);

    if(n < 0)
      err_sys("INCORRECT INPUT. children number should be greater than zero");

    int input_fd = open(argv[1], O_RDONLY);

    if(input_fd < 0)
      err_sys("CAN'T OPEN INPUT FILE");

    conn_t* connections = (conn_t*)calloc(n, sizeof(conn_t));

    int child_fds[2] = {};

    switch (connect(n, connections, child_fds, input_fd)) {
      case 1:
        fd_set writeable, readable;
        while (1)
        {
          int maxfd = -1;
          FD_ZERO(&writeable);
          FD_ZERO(&readable);

          for(int i = 0; i < n; i++)
          {
            if(connections[i].size == 0 && !connections[i].dead)
            {
              FD_SET(connections[i].fds[0], &readable);

              if(connections[i].fds[0] > maxfd)
                maxfd = connections[i].fds[0];
            }
            if(connections[i].size != 0)
            {
              FD_SET(connections[i].fds[1], &writeable);

              if(connections[i].fds[1] > maxfd)
                maxfd = connections[i].fds[1];
            }
          }
          if(maxfd < 0)
            break;

          if(!select(maxfd+1, &readable, &writeable, NULL, NULL))
            err_sys("SELECT ERROR");

          for(int i = 0; i < n; i++)
          {
            if(FD_ISSET(connections[i].fds[0], &readable))
            {
              int read_ret = read(connections[i].fds[0], connections[i].buf, connections[i].buf_capacity);

              if(read_ret < 0)
              err_sys("READ ERRSSOR");
              if(read_ret == 0)
              {
                connections[i].dead = 1;
                connections[i].size = 0;
                close(connections[i].fds[1]);
              }
              else
                connections[i].size = read_ret;
            }
            if(FD_ISSET(connections[i].fds[1], &writeable))
            {
              int write_ret = write(connections[i].fds[1], connections[i].buf + connections[i].offset, connections[i].size);
              if(write_ret < 0)
                err_sys("WRITE ERwROR");

              connections[i].size -= write_ret;
              if(connections[i].size == 0)
                connections[i].offset = 0;
              else
                connections[i].offset += write_ret;

            }

          }
        }
        close(input_fd);

        for(int i = 0; i < n; i++)
          free(connections[i].buf);
        break;

      case 0:
        char* child_buf = (char*)calloc(CHILDBUFSIZE, sizeof(char));
        int read_ret = 1;

        while(read_ret)
        {
          read_ret = read(child_fds[0], child_buf, CHILDBUFSIZE);
          if(read_ret < 0)
            err_sys("READ ERROR");
          write(child_fds[1], child_buf, read_ret);
        }
        close(child_fds[0]);
        close(child_fds[1]);
        free(child_buf);
        break;
    }
    free(connections);
    exit(0);
}

int connect(int n, conn_t* connections, int* child_fds, int first_child_read_fd)
{
  int fds1[2] = {};
  int fds2[2] = {};

  if(pipe(fds1) < 0)
    err_sys("PIPE ERROR");


  switch (fork())
  {
    case -1:
      err_sys("FORK ERROR");
    case 0:
      close(fds1[0]);
      child_fds[0] = first_child_read_fd;
      child_fds[1] = fds1[1];
      return 0;

    default:
      close(fds1[1]);
      connections[0].fds[0] = fds1[0];
  }

  for(int i = 0; i < n - 1; i++)
  {
    if(pipe(fds1) < 0)
      err_sys("PIPE ERROR");
    if(pipe(fds2) < 0)
        err_sys("PIPE ERROR");

    switch (fork())
    {
      case -1:
        err_sys("FORK ERROR");
      case 0:
        close(fds1[1]);
        close(fds2[0]);
        child_fds[0] = fds1[0];
        child_fds[1] = fds2[1];

        for(int j = 0; j < i; j++)
        {
          close(connections[j].fds[0]);
          close(connections[j].fds[1]);
        }
        return 0;

      default:
        close(fds1[0]);
        close(fds2[1]);

        fcntl(fds1[1], F_SETFL, O_WRONLY | O_NONBLOCK);
        fcntl(fds2[0], F_SETFL, O_RDONLY | O_NONBLOCK);

        connections[i].fds[1]   = fds1[1];
        connections[i+1].fds[0] = fds2[0];
    }
  }
  connections[n - 1].fds[1] = STDOUT_FILENO;

  for(int i = 0; i < n; i++)
  {
      connections[i].buf_capacity = ((n - i > 4) ? MAXCAPACITY : (int)pow(3, (double)(n - i))) * 1024;
      connections[i].size         = 0;
      connections[i].offset       = 0;
      connections[i].dead         = 0;
      connections[i].buf          = (char*)calloc(connections[i].buf_capacity, sizeof(char));


  }

  return 1;
}




void err_sys(const char* error)
{
	perror(error);
	exit(1);
}
