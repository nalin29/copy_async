#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>
#include <linux/io_uring.h>
#include <liburing.h>
#include <sys/ioctl.h>

struct ioEntry
{
   int reading;
   int fdSrc;
   int fdDest;
   int readOff;
   int last;
   int writeOff;
   char *srcName;
   char *destName;
   char *buffer;
   int readStatus;
   int writeStatus;
   struct aiocb *read_aiocb;
   struct aiocb *write_aiocb;
};

struct ioUringEntry
{
   int fdSrc;
   int fdDest;
   int last;
   int read;
   int readOff;
   int writeOff;
   char* buffer;
   int buff_index;
   off_t file_size;
   int op_count;
   struct iovec* iov;
};