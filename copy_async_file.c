
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


static volatile sig_atomic_t gotSIGQUIT = 0;
/* On delivery of SIGQUIT, we attempt to
   cancel all outstanding I/O requests */

static void /* Handler for SIGQUIT */
quitHandler(int sig)
{
   gotSIGQUIT = 1;
}

#define IO_SIGNAL SIGUSR1 /* Signal used to notify I/O completion */

static void /* Handler for I/O completion signal */
aioSigHandler(int sig, siginfo_t *si, void *ucontext)
{
   if (si->si_code == SI_ASYNCIO)
   {
      write(STDOUT_FILENO, "I/O completion signal received\n", 31);

     struct ioEntry *info = si->si_value.sival_ptr;

      printf("Information: %p\n", info);
      fflush(stdout);

      /*if (!info->reading)
      {
         int status = aio_error(info->write_aiocb);
         int num_write = aio_return(info->write_aiocb);

         if (status != 0 || num_write == 0)
            return;

         info->writeOff += num_write;
         info->reading = 1;

         info->read_aiocb->aio_offset = info->readOff;
         info->readStatus = EINPROGRESS;
         printf("Starting Read\n");
         fflush(stdout);
         CHECK_ERROR(aio_read(info->read_aiocb), "async read in handler");
      } */
   }
}

void install_aio_signal_handler()
{
   struct sigaction sa;
   sa.sa_flags = SA_SIGINFO | SA_RESTART;
   sigemptyset(&sa.sa_mask);
   sa.sa_sigaction = aioSigHandler;
   CHECK_ERROR(sigaction(IO_SIGNAL, &sa, NULL), "Installing aio handler");
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

   int numReq = 1;
   int openReq = 0;
   struct ioEntry *ioList;
   struct aiocb *aiocbList;
   char *bufList;
   char *fileNameList;

   ioList = calloc(numReq, sizeof(struct ioEntry));
   CHECK_NULL(ioList, "Allocating IOList");
   aiocbList = calloc(2 * numReq, sizeof(struct aiocb));
   CHECK_NULL(aiocbList, "Allocating aiocb list");
   bufList = calloc(numReq, BUFF_SIZE);
   CHECK_NULL(bufList, "Allocating buflist");
   fileNameList = calloc(2 * numReq, PATH_MAX);
   CHECK_NULL(fileNameList, "Allocating Filename List");

   //install_sigquit_signal_handler();
   //install_aio_signal_handler();

   int ret;
   for (int i = 0; i < numReq; i++)
   {
      ioList[i].srcName = &fileNameList[2 * i * PATH_MAX];
      ioList[i].destName = &fileNameList[(2 * i + 1) * PATH_MAX];
      ioList[i].buffer = &bufList[i * (BUFF_SIZE)];
      ioList[i].read_aiocb = &aiocbList[2 * i];
      ioList[i].write_aiocb = &aiocbList[2 * i + 1];

      ioList[i].readOff = 0;
      ioList[i].writeOff = 0;
      ioList[i].fdSrc = fd;
      ioList[i].fdDest = fdDest;
      ioList[i].readStatus = EINPROGRESS;
      ioList[i].writeStatus = EINPROGRESS;
      strncpy(ioList[i].srcName, src, PATH_MAX);
      strncpy(ioList[i].destName, dest, PATH_MAX);

      memset(ioList[i].read_aiocb, 0, sizeof(struct aiocb));
      memset(ioList[i].write_aiocb, 0, sizeof(struct aiocb));

      ioList[i].read_aiocb->aio_fildes = ioList[i].fdSrc;
      ioList[i].read_aiocb->aio_buf = ioList[i].buffer;
      ioList[i].read_aiocb->aio_nbytes = BUFF_SIZE;
      ioList[i].read_aiocb->aio_reqprio = 0;
      ioList[i].read_aiocb->aio_offset = 0;
      ioList[i].read_aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;
      ioList[i].read_aiocb->aio_sigevent.sigev_signo = IO_SIGNAL;
      ioList[i].read_aiocb->aio_sigevent.sigev_value.sival_ptr = &ioList[i];

      ioList[i].write_aiocb->aio_fildes = ioList[i].fdDest;
      ioList[i].write_aiocb->aio_buf = ioList[i].buffer;
      ioList[i].write_aiocb->aio_nbytes = BUFF_SIZE;
      ioList[i].write_aiocb->aio_reqprio = 0;
      ioList[i].write_aiocb->aio_offset = 0;
      ioList[i].write_aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;
      ioList[i].write_aiocb->aio_sigevent.sigev_signo = IO_SIGNAL;
      ioList[i].write_aiocb->aio_sigevent.sigev_value.sival_ptr = &ioList[i];

      //printf("Location is %p\n", &ioList[i]);
   }

   for (int i = 0; i < numReq; i++)
   {
      openReq++;
      ioList[i].reading = 1;
      //printf("I/O Read #%d\n", i);
      //fflush(stdout);
      ret = aio_read(ioList[i].read_aiocb);
   }

   // add code to cancel async
   while (openReq > 0)
   {
      if (gotSIGQUIT)
      {
         exit(0);
      }

      for (int i = 0; i < numReq; i++)
      {
         if (ioList[i].readStatus == EINPROGRESS)
         {
            ioList[i].readStatus = aio_error(ioList[i].read_aiocb);
            switch (ioList[i].readStatus)
            {
            case 0:
               // write(STDOUT_FILENO, "I/O completion signal received\n", 31);
               int num_read = aio_return(ioList[i].read_aiocb);
               ioList[i].readOff += num_read;
               ioList[i].reading = 0;

               //printf("Read %d \n", num_read);

               if(num_read == 0){
                  //printf("Finished copying\n");
                  openReq--;
                  break;
               }
               //printf("Starting write\n");
               //fflush(stdout);
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
         else if(ioList[i].writeStatus == EINPROGRESS){
            ioList[i].writeStatus = aio_error(ioList[i].write_aiocb);
            switch (ioList[i].writeStatus)
            {
            case 0:
               // write(STDOUT_FILENO, "I/O completion signal received\n", 31);
               int num_write = aio_return(ioList[i].write_aiocb);
               ioList[i].writeOff += num_write;
               ioList[i].reading = 0;

               //printf("Starting read\n");
               //fflush(stdout);
               ioList[i].read_aiocb->aio_offset = ioList[i].readOff;
               ioList[i].readStatus = EINPROGRESS;
               CHECK_ERROR(aio_read(ioList[i].read_aiocb), "async read");
               break;
            case EINPROGRESS:
               break;
            default:
               perror("aio_read error");
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