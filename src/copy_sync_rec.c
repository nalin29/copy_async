/**
 * @file copy_sync_rec.c
 * @author Nalin Mahajan, Vineeth Bandi
 * @brief A stripped down version of copy -r that can be used as comparison.
 * @version 0.1
 * @date 2022-04-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "copy.h"
#include "list.h"
#include "logging.h"

char *src;
char *dest;
int src_name_size;
int f_opt = 0;
int d_opt = 0;
int writeFlags = O_WRONLY | O_CREAT | O_EXCL;
int readFlags = O_RDONLY | O_NOFOLLOW;

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
      // if empty return null
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
         free(name);
      }
      else
      {
         strcpy(src_file, name);
         get_dest(src_file, dest_file);
         free(name);
         break;
      }
   }
}
// a stripped down versio of what cp -r does
void copy_sync(struct list *queue)
{
   char src_path[PATH_MAX + 1];
   char dest_path[PATH_MAX + 1];
   while (1)
   {
      // gather file to open
      getNextFile(queue, src_path, dest_path);
      if (dest_path[0] == '\0')
      {
         break;
      }

      // open file
      int infd = openat(AT_FDCWD, src_path, readFlags);
      int outfd = openat(AT_FDCWD, dest_path, writeFlags, 0644);

      // stat file and fallocate if option enabled
      struct stat file_stat;
      CHECK_ERROR(fstat(infd, &file_stat), "stat");

      if (f_opt)
      {

         off_t file_size = file_stat.st_size;

         CHECK_ERROR(fallocate(outfd, 0, 0, file_size), "fallocate file");
      }

      // keep copying till no more left or erro
      while (1)
      {
         ssize_t ret;
         ret = copy_file_range(infd, NULL, outfd, NULL, 9223372035781033984, 0);
         CHECK_ERROR(ret, "copy file range");
         if (ret == 0)
            break;
      }
      // set permissions
      CHECK_ERROR(fchmod(outfd, file_stat.st_mode), "set permissions");
      close(infd);
      close(outfd);
   }
}

void print_usage()
{
   printf("Usage: cp_sync_rec SOURCE DEST [OPTION]\n");
   printf("Recursivly copy source folder to new destination folder\n\n");
   printf("Optional Flags:\n");
   printf("-h\t\tTo bring up help menu\n");
   printf("-f\t\tenables use of fallocate\n");
   printf("-d\t\tDirectly addresses storage device, no-buffering (all file size must multiple of 4096)\n");
   exit(0);
}

int main(int argc, char **argv)
{
   struct timespec start, end;
   struct rusage r_usage;
   

   CHECK_ERROR(clock_gettime(CLOCK_MONOTONIC, &start), "getting start time");

   // arg1 directory structure to copy, arg2 destination directory
   if (argc < 3)
   {
      printf("Need to have src and dest\n");
      print_usage();
   }

   // gather and set flags
   for (int i = 3; i < argc; i++)
   {
      if (strcmp(argv[i], "-f") == 0)
      {
         f_opt = 1;
      }
      else if (strcmp(argv[i], "-d") == 0)
      {
         d_opt = 1;
         readFlags |= O_DIRECT;
         writeFlags |= O_DIRECT;
      }
      else if (strcmp(argv[i], "-h") == 0)
      {
         print_usage();
      }
   }

   src = argv[1];
   dest = argv[2];

   src_name_size = strlen(src);

   DIR *src_dir;
   DIR *dest_dir;

   // open and check src/dest dir
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

   // execute requested copy operations
   copy_sync(queue);

   getrusage(RUSAGE_SELF,&r_usage);

   clock_gettime(CLOCK_MONOTONIC, &end);
   double time_taken;
   time_taken = (end.tv_sec - start.tv_sec) * 1e9;
   time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
   // printf("The elapsed time is %f seconds\n", time_taken);
   printf("%f,", time_taken);
   printf("%ld\n", r_usage.ru_maxrss);

   closedir(src_dir);
}
