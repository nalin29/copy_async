#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>

#define BUFF_SIZE (128 * 1024)

#define MAX_FILES 100

struct ioEntry
{
   int reading;
   int fdSrc;
   int fdDest;
   int readOff;
   int writeOff;
   char *srcName;
   char *destName;
   char *buffer;
   int readStatus;
   int writeStatus;
   struct aiocb *read_aiocb;
   struct aiocb *write_aiocb;
};