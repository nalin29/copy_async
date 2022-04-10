#define _GNU_SOURCE 

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "copy.h"
#include "logging.h"
int MAX_FILES = 128;
int BUFF_SIZE = (128 * 1024);
int main(int argc, char ** argv){
   char* src = argv[1];
   char* dst = argv[2];

   int fd_src = open(src, O_RDONLY);
   int fd_dst = open(dst, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

   struct ioEntry* entry = malloc(sizeof(struct ioEntry));

   entry->fdSrc= fd_src;
   entry->fdDest = fd_dst;
   entry->buffer = malloc(BUFF_SIZE);
   entry->read_aiocb = malloc(sizeof(struct aiocb));
   entry->write_aiocb = malloc(sizeof(struct aiocb));
   entry->reading = 1;
   entry->readOff = 0;
   entry->writeOff = 0;

   memset(entry->read_aiocb, 0 , sizeof(struct aiocb));
   memset(entry->write_aiocb, 0 , sizeof(struct aiocb));

   entry->read_aiocb->aio_buf = entry->buffer;
   entry->read_aiocb->aio_fildes = entry->fdSrc;
   entry->read_aiocb->aio_nbytes = BUFF_SIZE;
   entry->read_aiocb->aio_offset = 0;

   entry->write_aiocb->aio_buf = entry->buffer;
   entry->write_aiocb->aio_fildes = entry->fdDest;
   entry->write_aiocb->aio_offset = 0;

   entry->readStatus = EINPROGRESS;
   aio_read(entry->read_aiocb);

   while(1){
      if(entry->readStatus == EINPROGRESS){
         struct aiocb* cb = entry->read_aiocb;
         entry->readStatus = aio_error(cb);
         if(entry->readStatus == 0){
            int num_read = aio_return(cb);
            if(num_read == 0){
               printf("done\n");
               break;
            }

            entry->readOff += num_read;
            entry->write_aiocb->aio_offset = entry->writeOff;
            entry->write_aiocb->aio_nbytes = num_read;
            entry->writeStatus = EINPROGRESS;
            aio_write(entry->write_aiocb);
         }
      }
      else if(entry->writeStatus == EINPROGRESS){
         struct aiocb* cb = entry->write_aiocb;
         entry->writeStatus = aio_error(cb);
         if(entry->writeStatus == 0){
            int num_write = aio_return(cb);

            entry->writeOff += num_write;
            
            entry->read_aiocb->aio_nbytes = BUFF_SIZE;
            entry->read_aiocb->aio_offset = entry->readOff;
            entry->readStatus = EINPROGRESS;

            aio_read(entry->read_aiocb);

         }
      }
   }
   close(fd_src);
   close(fd_dst);
}