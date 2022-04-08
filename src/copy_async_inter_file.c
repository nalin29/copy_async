
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
#include "list.h"
#include "logging.h"

static volatile sig_atomic_t gotSIGQUIT = 0;
/* On delivery of SIGQUIT, we attempt to
   cancel all outstanding I/O requests */

static void /* Handler for SIGQUIT */
quitHandler(int sig)
{
   gotSIGQUIT = 1;
}

void install_sigquit_signal_handler()
{
   struct sigaction sa;
   sa.sa_flags = SA_RESTART;
   sigemptyset(&sa.sa_mask);
   sa.sa_handler = quitHandler;
   CHECK_ERROR(sigaction(SIGQUIT, &sa, NULL), "Installing SiqQuit Handler");
}

int main(int argc, char **argv)
{
   double time_spent = 0.0;
   clock_t begin = clock();

   if (argc < 3)
   {
      printf("Need to have src and dest\n");
      return -1;
   }
   char *src = argv[1];
   char *dest = argv[2];
   int fd = open(src, O_RDONLY);
   CHECK_ERROR(fd, "Open SRC");
   int fdDest = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
   CHECK_ERROR(fdDest, "OPEN DEST");
   struct stat file_stat;
   CHECK_ERROR(fstat(fd, &file_stat), "stat");
   int file_size = file_stat.st_size;
   CHECK_ERROR(fallocate(fdDest, 0, 0, file_size), "fallocate file");
   int files = 1;
   int numReq = (file_size + BUFF_SIZE - 1) / BUFF_SIZE;
   int openReq = 0;
   struct ioEntry *ioList;
   struct aiocb *aiocbList;
   char *bufList;
   char *fileNameList;

   // add start that detects mem size allocates approx 1/2 memory

   ioList = calloc(numReq, sizeof(struct ioEntry));
   CHECK_NULL(ioList, "Allocating IOList");
   aiocbList = calloc(2 * numReq, sizeof(struct aiocb));
   CHECK_NULL(aiocbList, "Allocating aiocb list");
   bufList = calloc(numReq, BUFF_SIZE);
   CHECK_NULL(bufList, "Allocating buflist");
   fileNameList = calloc(2 * files, PATH_MAX + 1);
   CHECK_NULL(fileNameList, "Allocating Filename List");

   // add method call that will bfs through directories and build queue of files and directories

   install_sigquit_signal_handler();

   // need to change this to operate on linked list
   // also needs deque files from the bfs queue (if directory then mkdir)
   for (int j = 0; j < files; j++)
   {
      for (int i = 0; i < numReq; i++)
      {
         // set pointers
         ioList[i].srcName = &fileNameList[2 * j * (PATH_MAX+1)];
         ioList[i].destName = &fileNameList[(2 * j + 1) * (PATH_MAX+1)];
         ioList[i].buffer = &bufList[i * (BUFF_SIZE)];
         ioList[i].read_aiocb = &aiocbList[2 * i];
         ioList[i].write_aiocb = &aiocbList[2 * i + 1];

         // set values
         ioList[i].readOff = i * BUFF_SIZE;
         ioList[i].writeOff = i * BUFF_SIZE;
         ioList[i].fdSrc = fd;
         ioList[i].fdDest = fdDest;
         ioList[i].readStatus = EINPROGRESS;
         ioList[i].writeStatus = EINPROGRESS;
         strncpy(ioList[i].srcName, src, PATH_MAX);
         strncpy(ioList[i].destName, dest, PATH_MAX);

         memset(ioList[i].read_aiocb, 0, sizeof(struct aiocb));
         memset(ioList[i].write_aiocb, 0, sizeof(struct aiocb));

         // set values for aio control blocks
         ioList[i].read_aiocb->aio_fildes = ioList[i].fdSrc;
         ioList[i].read_aiocb->aio_buf = ioList[i].buffer;
         ioList[i].read_aiocb->aio_nbytes = BUFF_SIZE;
         ioList[i].read_aiocb->aio_reqprio = 0;
         ioList[i].read_aiocb->aio_offset = ioList[i].readOff;
         ioList[i].read_aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;
         ioList[i].read_aiocb->aio_sigevent.sigev_signo = 0;
         ioList[i].read_aiocb->aio_sigevent.sigev_value.sival_ptr = &ioList[i];

         ioList[i].write_aiocb->aio_fildes = ioList[i].fdDest;
         ioList[i].write_aiocb->aio_buf = ioList[i].buffer;
         ioList[i].write_aiocb->aio_nbytes = BUFF_SIZE;
         ioList[i].write_aiocb->aio_reqprio = 0;
         ioList[i].write_aiocb->aio_offset = ioList[i].writeOff;
         ioList[i].write_aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;
         ioList[i].write_aiocb->aio_sigevent.sigev_signo = 0;
         ioList[i].write_aiocb->aio_sigevent.sigev_value.sival_ptr = &ioList[i];
      }
   }

   for (int i = 0; i < numReq; i++)
   {
      openReq++;
      ioList[i].reading = 1;
      // printf("I/O Read #%d\n", i);
      // fflush(stdout);
      CHECK_ERROR(aio_read(ioList[i].read_aiocb), "Starting First AIO Read");
   }

   // add code to cancel async
   while (openReq > 0)
   {
      if (gotSIGQUIT)
      {
         exit(0);
      }

      // operate on a linked list
      // when an operation finishes get next value in queue and install + run, (this may cause a bfs search if queue empty of files)
      // if out of files then remove from linked list and dec open_req
      for (int i = 0; i < numReq; i++)
      {
         if (ioList[i].readStatus == EINPROGRESS)
         {
            ioList[i].readStatus = aio_error(ioList[i].read_aiocb);
            switch (ioList[i].readStatus)
            {
               int num_read;
            case 0:
               // write(STDOUT_FILENO, "I/O completion signal received\n", 31);
               num_read = aio_return(ioList[i].read_aiocb);
               ioList[i].readOff += num_read;
               ioList[i].reading = 0;

               // printf("Read %d \n", num_read);
               // printf("Starting write\n");
               // fflush(stdout);
               ioList[i].write_aiocb->aio_offset = ioList[i].writeOff;
               ioList[i].write_aiocb->aio_nbytes = num_read;
               ioList[i].writeStatus = EINPROGRESS;
               CHECK_ERROR(aio_write(ioList[i].write_aiocb), "async write");
               break;
            case EINPROGRESS:
               break;
            default:
               perror("aio_read error");
               exit(-1);
               break;
            }
         }
         else if (ioList[i].writeStatus == EINPROGRESS)
         {
            ioList[i].writeStatus = aio_error(ioList[i].write_aiocb);
            switch (ioList[i].writeStatus)
            {
               int num_write;
            case 0:
               // write(STDOUT_FILENO, "I/O completion signal received\n", 31);
               num_write = aio_return(ioList[i].write_aiocb);
               ioList[i].writeOff += num_write;
               ioList[i].reading = 0;

               // printf("Starting read\n");
               // fflush(stdout);
               // printf("Finished copying\n");
               openReq--;
               break;
            case EINPROGRESS:
               break;
            default:
               perror("aio_write error");
               exit(-1);
               break;
            }
         }
      }
   }
   clock_t end = clock();
   time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
   printf("The elapsed time is %f seconds", time_spent);
   close(fd);
   close(fdDest);
}