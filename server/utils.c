#include "utils.h"

int _strrpl(const char *src, char **dst) {
    if (!(src || dst)) { return -1; }
    
    if (*dst && strlen(src) == strlen(*dst)) {
        memset(*dst, '\0', strlen(*dst));
    } else {
        char *old = *dst;
        *dst = (char *)malloc((strlen(src) + 1) * sizeof(char));
        if (!(*dst)) {
            *dst = old; 
            return -1; 
        }
        free(old);
    }

    *dst = strcpy(*dst, src);
    return 0;
}