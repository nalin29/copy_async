#include <stdio.h>
#include <errno.h>

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
