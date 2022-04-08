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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "copy.h"
#include "list.h"
#include "logging.h"

static volatile sig_atomic_t gotSIGQUIT = 0;
char *src;
char *dest;
int src_name_size;
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

// given a filename with path ../src/.../name will convert to ../dest/../name
void get_dest(char *src_name, char *dest_name)
{
   CHECK_NULL(strcpy(dest_name, dest), "copying dest top file name");
   CHECK_NULL(strncat(dest_name, src_name + src_name_size, PATH_MAX), "concat filename relative to source");
};

// sets dest_file to correct value, if empty dest_file = NULL
void getNextFile(struct list *queue, char *src_file, char *dest_file)
{
   // perform bfs
   while (1)
   {
      // printf("%d\n",queue->size);
      if (queue->size == 0)
      {
         CHECK_ERROR(memset(dest_file, 0, PATH_MAX), "null dest");
         return;
      }
      char *name = (char *)deq_node(queue);

      struct stat path_stat;
      CHECK_ERROR(stat(name, &path_stat), "stat file");

      get_dest(name, dest_file);
      // if file is directory mkdir at correct relative point
      // traverse directory and append files/dir to end of queue
      if (S_ISDIR(path_stat.st_mode))
      {
         CHECK_ERROR(mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO), "making new directory");
         DIR *dir;
         struct dirent *dp;
         dir = opendir(name);
         CHECK_NULL(dir, "opening orignal source dir");
         while ((dp = readdir(dir)) != NULL)
         {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
               continue;
            char *file_name = malloc(PATH_MAX * sizeof(char));
            CHECK_NULL(strcpy(file_name, name), "copying in string");
            CHECK_NULL(strcat(file_name, "/"), "concating /");
            CHECK_NULL(strcat(file_name, dp->d_name), "concat file name to src dir");
            enq_node(queue, (void *)file_name);
         }
         closedir(dir);
      }
      else
      {
         strcpy(src_file, name);
         get_dest(src_file, dest_file);
         break;
      }
   }
}

int main(int argc, char **argv)
{
   double time_spent = 0.0;
   clock_t begin = clock();

   // arg1 directory structure to copy, arg2 destination directory
   if (argc < 3)
   {
      printf("Need to have src and dest\n");
      return -1;
   }

   src = argv[1];
   dest = argv[2];

   src_name_size = strlen(src);

   DIR *src_dir;
   DIR *dest_dir;

   src_dir = opendir(src);
   CHECK_NULL(src_dir, "Opening Src dir");

   dest_dir = opendir(dest);
   if (dest_dir != NULL)
   {
      printf("Directory already exists");
      closedir(dest_dir);
      exit(0);
   }
   else if (errno != ENOENT)
   {
      perror("ERROR: ");
      exit(0);
   }

   // set up queue
   struct list *queue = malloc(sizeof(struct list));
   CHECK_NULL(queue, "Allocating queue");
   queue->size = 0;
   queue->head = NULL;
   queue->tail = NULL;

   // push onto queue

   enq_node(queue, src);

   // working set list

   struct list *iolist = malloc(sizeof(struct list));
   CHECK_NULL(iolist, "Allocating iolist");
   iolist->size = 0;
   iolist->head = NULL;
   iolist->tail = NULL;

   // dequeue items and add them to list
   // loop and add items + fallocate + run
   char src_name[PATH_MAX];
   char dest_name[PATH_MAX];
   for (int i = 0; i < MAX_FILES; i++)
   {

      getNextFile(queue, src_name, dest_name);
      if (dest_name[0] == '\0')
      {
         break;
      }

      struct ioEntry *entry = malloc(sizeof(struct ioEntry));
      CHECK_NULL(entry, "Allocating ioEntry");

      char *buffer = malloc(BUFF_SIZE);
      CHECK_NULL(buffer, "Allocating buffer");

      struct aiocb *read_aiocb = malloc(sizeof(struct aiocb));
      CHECK_NULL(read_aiocb, "Allocating aiocb");

      struct aiocb *write_aiocb = malloc(sizeof(struct aiocb));
      CHECK_NULL(write_aiocb, "Allocating aiocb");

      int fd_src = open(src_name, O_RDONLY | O_DIRECT);
      CHECK_ERROR(fd_src, "Opening src file");
      int fd_dest = open(dest_name, O_WRONLY | O_CREAT | O_DIRECT, S_IRWXU | S_IRWXO | S_IRWXG);
      CHECK_ERROR(fd_dest, "Opening dest file");

      // fallocate
      /*       struct stat file_stat;
            CHECK_ERROR(fstat(fd_src, &file_stat), "stat");
            int file_size = file_stat.st_size;
            CHECK_ERROR(fallocate(fd_dest, 0, 0, file_size), "fallocate file"); */

      entry->reading = 1;
      entry->fdSrc = fd_src;
      entry->fdDest = fd_dest;
      entry->readOff = 0;
      entry->writeOff = 0;
      entry->buffer = buffer;
      entry->read_aiocb = read_aiocb;
      entry->write_aiocb = write_aiocb;

      memset(read_aiocb, 0, sizeof(struct aiocb));
      memset(write_aiocb, 0, sizeof(struct aiocb));

      read_aiocb->aio_buf = entry->buffer;
      read_aiocb->aio_fildes = entry->fdSrc;
      read_aiocb->aio_nbytes = BUFF_SIZE;
      read_aiocb->aio_offset = 0;

      write_aiocb->aio_buf = entry->buffer;
      write_aiocb->aio_fildes = entry->fdDest;
      write_aiocb->aio_offset = 0;

      enq_node(iolist, entry);
   }

   int openReq = 0;

   struct node *cur = iolist->head;
   for (int idx = 0; idx < iolist->size; idx++)
   {
      openReq++;
      struct ioEntry *entry = (struct ioEntry *)cur->data;
      entry->readStatus = EINPROGRESS;
      CHECK_ERROR(aio_read(entry->read_aiocb), "starting read aio");
      cur = cur->next;
   }

   // then start while loop
   cur = iolist->head;
   while (openReq > 0)
   {
      if (cur == NULL)
         break;
      struct ioEntry *entry = (struct ioEntry *)cur->data;
      if (entry->readStatus == EINPROGRESS)
      {
         entry->readStatus = aio_error(entry->read_aiocb);
         switch (entry->readStatus)
         {
         case 0:
            int num_read;
            num_read = aio_return(entry->read_aiocb);
            if (num_read == 0)
            {
               CHECK_ERROR(close(entry->fdSrc), "closing src");
               CHECK_ERROR(close(entry->fdDest), "closing dest");
               // need to open next file and replace values in entry (and start execution)

               getNextFile(queue, src_name, dest_name);

               if (dest_name[0] == '\0')
               {
                  // if empty then decrement and remove from openReq
                  struct node *temp = cur->prev;
                  remove_node(iolist, cur);
                  cur = temp;
                  openReq--;
                  break;
               }
               else
               {
                  int fd_src = open(src_name, O_RDONLY | O_DIRECT);
                  CHECK_ERROR(fd_src, "Opening src file");
                  int fd_dest = open(dest_name, O_WRONLY | O_CREAT | O_DIRECT, S_IRWXU | S_IRWXO | S_IRWXG);
                  CHECK_ERROR(fd_dest, "Opening dest file");

                  /* struct stat file_stat;
                  CHECK_ERROR(fstat(fd_src, &file_stat), "stat");
                  int file_size = file_stat.st_size;
                  CHECK_ERROR(fallocate(fd_dest, 0, 0, file_size), "fallocate file"); */

                  entry->fdDest = fd_dest;
                  entry->fdSrc = fd_src;
                  entry->readOff = 0;
                  entry->writeOff = 0;

                  memset(entry->read_aiocb, 0, sizeof(struct aiocb));
                  memset(entry->write_aiocb, 0, sizeof(struct aiocb));

                  entry->read_aiocb->aio_fildes = fd_src;
                  entry->read_aiocb->aio_offset = 0;
                  entry->read_aiocb->aio_buf = entry->buffer;
                  entry->read_aiocb->aio_nbytes = BUFF_SIZE;

                  entry->write_aiocb->aio_offset = 0;
                  entry->write_aiocb->aio_fildes = fd_dest;
                  entry->write_aiocb->aio_buf = entry->buffer;

                  entry->readStatus = EINPROGRESS;
                  CHECK_ERROR(aio_read(entry->read_aiocb), "async read");
                  break;
               }
            }
            else
            {
               entry->readOff += num_read;
               entry->write_aiocb->aio_offset = entry->writeOff;
               entry->write_aiocb->aio_nbytes = num_read;
               entry->writeStatus = EINPROGRESS;
               CHECK_ERROR(aio_write(entry->write_aiocb), "async write");
               break;
            }
         case EINPROGRESS:
            break;
         default:
            perror("aio_read error");
            exit(-1);
            break;
         }
      }
      else if (entry->writeStatus == EINPROGRESS)
      {
         entry->writeStatus = aio_error(entry->write_aiocb);
         switch (entry->writeStatus)
         {
         case 0:
            // write(STDOUT_FILENO, "I/O completion signal received\n", 31);
            int num_write = aio_return(entry->write_aiocb);

            entry->writeOff += num_write;

            entry->read_aiocb->aio_offset = entry->readOff;
            entry->read_aiocb->aio_nbytes = BUFF_SIZE;
            entry->readStatus = EINPROGRESS;
            CHECK_ERROR(aio_read(entry->read_aiocb), "async read");
            break;
         case EINPROGRESS:
            break;
         default:
            perror("aio_read error");
            exit(-1);
            break;
         }
      }
      cur = cur->next;
   }

   clock_t end = clock();
   time_spent += (double)(end - begin) / (double)CLOCKS_PER_SEC;
   printf("The elapsed time is %f seconds\n", time_spent);

   closedir(src_dir);
}