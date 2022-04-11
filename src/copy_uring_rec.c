/**
 * @file copy_uring_rec.c
 * @author Nalin Mahajan Vineeth Bandi
 * @brief This file contains the code for recursive copy operations on linux using io_uring.
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <linux/io_uring.h>
#include <sys/time.h>
#include <liburing.h>
#include <sys/mman.h>
#include "copy.h"
#include "list.h"
#include "logging.h"

off_t QD = 64;           // Max number of concurrent ops
off_t BS = (128 * 1024); // size of buffer

int f_opt = 0;     // fallocate option
int rb_opt = 0;    // registered buffers option
int batch_opt = 0; // batching option
int inter_opt = 0; // inter-file operations option

int read_flags = O_RDONLY;            // flags for reading
int write_flags = O_CREAT | O_WRONLY; // flags for writing

char *src;         // src base path
char *dest;        // dest base path
int src_name_size; // size of src name

struct iovec *iovecs; // pointer to list iovecs
char *buffers;        // pointer to list of buffers

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
      // if empty return empty dest
      if (queue->size == 0)
      {
         CHECK_ERROR(strcpy(dest_file, "\0"), "null dest");
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
         free(name);
      }
      // otherwise copy name to src and generate dest name and return
      else
      {
         strcpy(src_file, name);
         get_dest(src_file, dest_file);
         free(name);
         break;
      }
   }
}

/*
   Generates a linked read then write pair operation and inserts into the SQ
*/
void add_rw_pair(struct io_uring *ring, struct ioUringEntry *entry)
{
   // get aqe
   struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
   CHECK_NULL(sqe, "getting sqe");

   // set link flag for write operation
   io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK);

   // prep read to sq
   if (rb_opt)
   {
      io_uring_prep_read_fixed(sqe, entry->fdSrc, entry->buffer, entry->iov->iov_len, entry->readOff, entry->buff_index);
   }
   else
   {
      io_uring_prep_readv(sqe, entry->fdSrc, entry->iov, 1, entry->readOff);
   }

   // set data to the entry
   io_uring_sqe_set_data(sqe, entry);

   // get next sqe (will have been linked to occur after the above op)
   sqe = io_uring_get_sqe(ring);
   CHECK_NULL(sqe, "getting sqe");

   // prep write to sq
   if (rb_opt)
   {
      io_uring_prep_write_fixed(sqe, entry->fdDest, entry->buffer, entry->iov->iov_len, entry->writeOff, entry->buff_index);
   }
   else
   {
      io_uring_prep_writev(sqe, entry->fdDest, entry->iov, 1, entry->writeOff);
   }

   io_uring_sqe_set_data(sqe, entry);
}

// normal rec copy with read/write entry per file
void rec_copy(struct list *queue, struct io_uring *ring)
{
   long in_flight = 0;
   char src_path[PATH_MAX + 1];
   char dest_path[PATH_MAX + 1];
   // generate intial sq ring
   for (int i = 0; i < QD; i += 2)
   {

      // get next file check for end of queue
      getNextFile(queue, src_path, dest_path);
      if (dest_path[0] == '\0')
      {
         break;
      }

      // generate entry
      struct ioUringEntry *entry = malloc(sizeof(struct ioUringEntry));
      CHECK_NULL(entry, "allocating an entry");

      // open file
      int infd = open(src_path, read_flags);
      CHECK_ERROR(infd, "opening read file");
      int outfd = open(dest_path, write_flags, S_IRWXU | S_IRWXO | S_IRWXG);
      CHECK_ERROR(outfd, "opening write file");

      // get file size for fallocate
      struct stat file_stat;
      CHECK_ERROR(fstat(infd, &file_stat), "stat");
      off_t file_size = file_stat.st_size;

      if (f_opt)
      {
         CHECK_ERROR(fallocate(outfd, 0, 0, file_size), "fallocate file");
      }

      // set file size and entry fields
      entry->file_size = file_size;

      entry->fdSrc = infd;
      entry->fdDest = outfd;

      entry->op_count = 0;
      entry->readOff = 0;
      entry->writeOff = 0;

      entry->iov = &iovecs[i / 2];

      // calculate then length of first read/write
      entry->iov->iov_len = file_size > BS ? BS : file_size;
      entry->buffer = entry->iov->iov_base;
      entry->buff_index = i / 2;

      // add pair
      add_rw_pair(ring, entry);

      in_flight += 2;
   }

   // submit ring starting read/write execution
   CHECK_ERROR(io_uring_submit(ring), "submitting ring");

   // reap the entries and start new ones
   while (in_flight > 0)
   {
      // gather cqe for previous sqe
      struct io_uring_cqe *cqe;
      CHECK_ERROR(io_uring_wait_cqe(ring, &cqe), "waiting on cqe");

      if (!cqe)
         break;

      struct ioUringEntry *entry;
      entry = (struct ioUringEntry *)io_uring_cqe_get_data(cqe);

      CHECK_NULL(entry, "getting entry from cqe");

      // check res code
      int res;
      res = cqe->res;
      if (res < 0)
      {
         printf("issue");
      }
      CHECK_ERROR(res, "result of cqe");

      in_flight--;
      entry->op_count++;
      // if read has finished increment readoff
      if (entry->op_count == 1)
      {
         entry->readOff += res;
      }
      // if read/write finished
      else if (entry->op_count == 2)
      {

         // remaining read/write iterate
         if (entry->readOff < entry->file_size)
         {
            entry->writeOff += res;
            entry->op_count = 0;
            off_t length = entry->file_size - entry->readOff;
            entry->iov->iov_len = length > BS ? BS : length;
            add_rw_pair(ring, entry);
            CHECK_ERROR(io_uring_submit(ring), "submitting ring");
            in_flight += 2;
         }
         // get next file
         else
         {
            close(entry->fdSrc);
            close(entry->fdDest);

            getNextFile(queue, src_path, dest_path);

            if (dest_path[0] == '\0')
            {
               free(entry);
            }
            else
            {
               int infd = open(src_path, read_flags);
               CHECK_ERROR(infd, "opening read file");
               int outfd = open(dest_path, write_flags, S_IRWXU | S_IRWXO | S_IRWXG);
               CHECK_ERROR(outfd, "opening write file");

               struct stat file_stat;
               CHECK_ERROR(fstat(infd, &file_stat), "stat");
               off_t file_size = file_stat.st_size;

               if (f_opt)
               {
                  CHECK_ERROR(fallocate(outfd, 0, 0, file_size), "fallocate file");
               }

               entry->file_size = file_size;

               entry->fdSrc = infd;
               entry->fdDest = outfd;

               entry->op_count = 0;
               entry->readOff = 0;
               entry->writeOff = 0;

               entry->iov->iov_len = file_size > BS ? BS : file_size;

               add_rw_pair(ring, entry);
               CHECK_ERROR(io_uring_submit(ring), "submitting ring");
               in_flight += 2;
            }
         }
      }
      // mark seen
      io_uring_cqe_seen(ring, cqe);
   }
}

// used for inter operations (intermediate queue of files that are being operated on)
// returns -1 in case where no such file
int enq_new_file(struct list *executingQueue, struct list *fileQueue)
{
   char src_path[PATH_MAX + 1];
   char dest_path[PATH_MAX + 1];
   // gets next file from file queue
   getNextFile(fileQueue, src_path, dest_path);
   if (dest_path[0] == '\0')
   {
      return -1;
   }
   struct file_entry *new_file;
   new_file = (struct file_entry *)malloc(sizeof(struct file_entry));
   CHECK_NULL(new_file, "allocating file entry");

   int infd = open(src_path, read_flags);
   CHECK_ERROR(infd, "opening read file");
   int outfd = open(dest_path, write_flags, S_IRWXU | S_IRWXO | S_IRWXG);
   CHECK_ERROR(outfd, "opening write file");

   struct stat file_stat;
   CHECK_ERROR(fstat(infd, &file_stat), "stat");
   off_t file_size = file_stat.st_size;

   if (f_opt)
   {
      CHECK_ERROR(fallocate(outfd, 0, 0, file_size), "fallocate file");
   }

   // set the needed fields and enq the new file to executing queue
   new_file->off = 0;
   new_file->infd = infd;
   new_file->outfd = outfd;
   new_file->remainingBytes = file_size;
   new_file->ops = 0;

   enq_node(executingQueue, new_file);
   return 0;
}

// gather next r/w pair for execution, returns -1 if now such file
int get_next_entry(struct list *executingQueue, struct list *fileQueue, struct ioUringEntry *entry)
{
   int ret;
   // if executing files is empty gather new file
   if (executingQueue->size == 0)
   {
      ret = enq_new_file(executingQueue, fileQueue);
      if (ret < 0)
         return ret;
   }

   // get first file in queue
   struct file_entry *head = (struct file_entry *)executingQueue->head->data;

   // if file consumed deq and enq new file
   if (head->remainingBytes <= 0)
   {
      deq_node(executingQueue);
      ret = enq_new_file(executingQueue, fileQueue);
      if (ret < 0)
         return ret;
   }

   // set entry data fields with data based on current file entry
   head = (struct file_entry *)executingQueue->head->data;

   entry->fdSrc = head->infd;
   entry->fdDest = head->outfd;
   entry->readOff = head->off;
   entry->writeOff = head->off;
   entry->iov->iov_len = head->remainingBytes > BS ? BS : head->remainingBytes;
   entry->fe = head;

   // update entry to reflect consumption
   head->off += entry->iov->iov_len;
   head->remainingBytes -= entry->iov->iov_len;
   head->ops += 1;

   // if comsumed deq and mark entry as last
   if (head->remainingBytes == 0)
   {
      entry->last = 1;
      deq_node(executingQueue);
   }
   else
      entry->last = 0;

   return 0;
}

void rec_copy_inter(struct list *queue, struct io_uring *ring)
{
   // create executingQueue
   struct list *executingQueue;
   executingQueue = (struct list *)malloc(sizeof(struct list));
   CHECK_NULL(executingQueue, "allocating queue for files in use");
   executingQueue->head = NULL;
   executingQueue->tail = NULL;
   executingQueue->size = 0;

   int in_flight = 0;

   int ret;
   // set up initial operations
   for (int i = 0; i < QD; i += 2)
   {
      // set entry struct fields
      struct ioUringEntry *entry;
      entry = (struct ioUringEntry *)malloc(sizeof(struct ioUringEntry));
      entry->iov = &iovecs[i / 2];
      entry->buff_index = i / 2;
      entry->buffer = entry->iov->iov_base;
      entry->op_count = 0;
      ret = get_next_entry(executingQueue, queue, entry);
      // if no more entries then done
      if (ret < 0)
      {
         free(entry);
         break;
      }
      // other wise add pair
      add_rw_pair(ring, entry);
      in_flight += 2;
   }

   // submit the operations
   CHECK_ERROR(io_uring_submit(ring), "submitting ring");

   // reap cqe and start new operations
   while (in_flight > 0)
   {
      struct io_uring_cqe *cqe;
      CHECK_ERROR(io_uring_wait_cqe(ring, &cqe), "waiting on cqe");

      if (!cqe)
         break;

      struct ioUringEntry *entry;
      entry = (struct ioUringEntry *)io_uring_cqe_get_data(cqe);

      CHECK_NULL(entry, "getting entry from cqe");

      int res;
      res = cqe->res;
      CHECK_ERROR(res, "result of cqe");

      in_flight--;
      entry->op_count++;
      // when done get next entry and restart otherwise free
      switch (entry->op_count)
      {
      case 2:
         // check in entry with fentry
         entry->fe->ops--;
         // if all ops checked in and no more future ops free file_entry and close files
         if(entry->fe->ops == 0 && entry->fe->remainingBytes == 0){
            free(entry->fe);
            close(entry->fdDest);
            close(entry->fdSrc);
         }
         entry->op_count = 0;
         ret = get_next_entry(executingQueue, queue, entry);
         if (ret == 0)
         {
            add_rw_pair(ring, entry);
            in_flight += 2;
            CHECK_ERROR(io_uring_submit(ring), "submitting ring");
         }
         else
         {
            free(entry);
         }
         break;
      default:
         break;
      }
      // mark seen
      io_uring_cqe_seen(ring, cqe);
   }
}

// copies used inter-file operations and batching
void rec_copy_batch(struct list *queue, struct io_uring *ring)
{
   // create executingQueue
   struct list *executingQueue;
   executingQueue = (struct list *)malloc(sizeof(struct list));
   CHECK_NULL(executingQueue, "allocating queue for files in use");
   executingQueue->head = NULL;
   executingQueue->tail = NULL;
   executingQueue->size = 0;

   int in_flight = 0;
   // set up initial operations
   int ret;
   for (int i = 0; i < QD; i += 2)
   {
      struct ioUringEntry *entry;
      entry = (struct ioUringEntry *)malloc(sizeof(struct ioUringEntry));
      entry->iov = &iovecs[i / 2];
      entry->buff_index = i / 2;
      entry->buffer = entry->iov->iov_base;
      entry->op_count = 0;
      ret = get_next_entry(executingQueue, queue, entry);
      if (ret < 0)
      {
         free(entry);
         break;
      }
      add_rw_pair(ring, entry);
      in_flight += 2;
   }

   while (in_flight > 0)
   {
      // submits all operations and waits till all complete
      struct io_uring_cqe *cqe;
      int num = io_uring_submit_and_wait(ring, in_flight);
      if (num != in_flight)
      {
         perror("Some operations are still in flight");
         exit(-1);
      }
      // loop through operations and reap
      for (int i = 0; i < num; i++)
      {
         ret = io_uring_peek_cqe(ring, &cqe);
         CHECK_ERROR(ret, "peeking cqe");

         if (!cqe)
            break;

         struct ioUringEntry *entry;
         entry = (struct ioUringEntry *)io_uring_cqe_get_data(cqe);

         CHECK_NULL(entry, "getting entry from cqe");

         int res;
         res = cqe->res;
         CHECK_ERROR(res, "result of cqe");

         in_flight--;
         entry->op_count++;
         // initialize next entry or free
         switch (entry->op_count)
         {
         case 2:
            // check in op with fe 
            entry->fe->ops--;
            // if all ops checked in and no more future ops free fe and close files
            if (entry->fe->ops == 0 && entry->fe->remainingBytes == 0)
            {
               free(entry->fe);
               close(entry->fdDest);
               close(entry->fdSrc);
            }
            entry->op_count = 0;
            ret = get_next_entry(executingQueue, queue, entry);
            if (ret == 0)
            {
               add_rw_pair(ring, entry);
               in_flight += 2;
            }
            else
            {
               free(entry);
            }
            break;
         default:
            break;
         }
         // mark seen
         io_uring_cqe_seen(ring, cqe);
      }
   }
}

void print_usage()
{
   printf("Usage: cp_uring_rec SOURCE DEST [OPTION]\n");
   printf("Recursivly copy source folder to new destination folder\n\n");
   printf("Optional Flags:\n");
   printf("-h\t\tTo bring up help menu\n");
   printf("-f\t\tenables use of fallocate\n");
   printf("-rb\t\tRegisters Buffers for zero copy (Warning: max size 4Mb (bs * qd/2))\n");
   printf("-i\t\tAllows mutiple concurrent operations on a single file\n");
   printf("-b\t\tBatches Requests (automatically enables -i)\n");
   printf("-d\t\tDirectly addresses storage device, no-buffering (all file size must multiple of 4096)\n");
   printf("-bs [int]\tSet size of buffer in Kb (default is 128kb)\n");
   printf("-qd [int]\tSet depth of Queue (Default is 64)\n");
   exit(0);
}

int main(int argc, char **argv)
{

   // timer

   struct timespec start, end;

   CHECK_ERROR(clock_gettime(CLOCK_MONOTONIC, &start), "getting start time");

   // arg1 directory structure to copy, arg2 destination directory
   if (argc < 3)
   {
      printf("Need to have src and dest\n");
      print_usage();
   }

   // set options
   for (int i = 3; i < argc; i++)
   {
      char *s = argv[i];
      if (strcmp(s, "-f") == 0)
      {
         f_opt = 1;
      }
      else if (strcmp(s, "-rb") == 0)
      {
         rb_opt = 1;
      }
      else if (strcmp(s, "-b") == 0)
      {
         batch_opt = 1;
      }
      else if (strcmp(s, "-i") == 0)
      {
         inter_opt = 1;
      }
      else if (strcmp(s, "-d") == 0)
      {
         read_flags |= O_DIRECT;
         write_flags |= O_DIRECT;
      }
      else if (strcmp(s, "-bs") == 0)
      {
         if (i + 1 < argc)
         {
            int x = atoi(argv[i + 1]);
            if (x == 0)
            {
               printf("invalid value for bs\n");
               print_usage();
            }
            BS = x * 1024;
         }
         else
         {
            printf("no value for BS\n");
            print_usage();
         }
      }
      else if (strcmp(s, "-qd") == 0)
      {
         if (i + 1 < argc)
         {
            int x = atoi(argv[i + 1]);
            if (x == 0)
            {
               printf("invalid value for qd\n");
               print_usage();
            }
            QD = x;
         }
         else
         {
            printf("no value for QD\n");
            print_usage();
         }
      }
      else if (strcmp(s, "-h") == 0)
      {
         print_usage();
      }
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

   // copy src into queue
   char *src_queue_data = malloc(PATH_MAX + 1);
   CHECK_NULL(src_queue_data, "allocating copy addr for src");
   CHECK_NULL(strcpy(src_queue_data, src), "copying src");
   enq_node(queue, src_queue_data);

   // setup io_uring
   struct io_uring *ring;

   ring = malloc(sizeof(struct io_uring));
   CHECK_NULL(ring, "allocating ring");

   CHECK_ERROR(io_uring_queue_init(QD, ring, 0), "init ring");

   // allocate buffers

   iovecs = calloc(QD / 2, (sizeof(struct iovec)));
   CHECK_NULL(iovecs, "allocating iovecs");

   buffers = mmap(NULL, (QD / 2) * BS, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
   CHECK_NULL(buffers, "allocating buffers");

   for (int i = 0; i < QD / 2; i++)
   {
      iovecs[i].iov_base = &buffers[i * BS];
      iovecs[i].iov_len = BS;
   }

   // register buffers
   if (rb_opt)
   {
      io_uring_register_buffers(ring, iovecs, QD / 2);
   }

   // run requested operation
   if (batch_opt)
   {
      rec_copy_batch(queue, ring);
   }
   else if (inter_opt)
   {
      rec_copy_inter(queue, ring);
   }
   else
   {
      rec_copy(queue, ring);
   }

   clock_gettime(CLOCK_MONOTONIC, &end);
   double time_taken;
   time_taken = (end.tv_sec - start.tv_sec) * 1e9;
   time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
   printf("The elapsed time is %f seconds\n", time_taken);

   closedir(src_dir);
   io_uring_queue_exit(ring);
   return 0;
}
