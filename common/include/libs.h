#ifndef __LIBS_H__
#define __LIBS_H__

#include <stdio.h>      //!< Printf etc
#include <stdlib.h>     //!< malloc etc
#include <string.h>     //!< strncmp etc
#include <stdint.h>     //!< Fixed-size types (uint32_t etc)
#include <stdarg.h>     //!< va_list etc
#include <time.h>       //!< Time functions
#include <stdbool.h>    //!< bool type
#include <assert.h>     //!< assert function

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <dirent.h>
#include <netinet/in.h>
#include <resolv.h>


#include <sys/queue.h>

#endif // __LIBS_H__
