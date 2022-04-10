/**
 * @file logging.h
 * @author Nalin Mahajan, Vineeth Bandi
 * @brief Logging macros
 * @version 0.1
 * @date 2022-04-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <errno.h>

// Error checking for syscalls who return -errno (-1)
#define CHECK_ERROR(ret, message) ({ \
   if (ret < 0)                      \
   {                                 \
      perror(message);               \
      exit(-1);                      \
   }                                 \
})

// error checking for pointer creation syscalls
#define CHECK_NULL(ret, message) ({ \
   if (ret == NULL)                 \
   {                                \
      perror(message);              \
      exit(-1);                     \
   }                                \
})

// error checking for value equivalence
#define COMPARE_VAL(out, val, message) ({ \
   if (out != val)                        \
   {                                      \
      perror(message);                    \
      exit(-1);                           \
   }                                      \
})
