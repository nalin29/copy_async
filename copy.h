#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>

#define BUFF_SIZE 128 * 1024

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

#define CHECK_ERROR(ret, message) ({ \
   if (ret < 0)                      \
   {                                 \
      perror(message);               \
      exit(-1);                      \
   }                                 \
   ret;                              \
})

#define CHECK_NULL(ret, message) ({ \
   if (ret == NULL)                 \
   {                                \
      perror(message);              \
      exit(-1);                     \
   }                                \
})

#define COMPARE_VAL(out, val, message) ({ \
   if (out != val)                        \
   {                                      \
      perror(message);                    \
      exit(-1);                           \
   }                                      \
})                                        \
