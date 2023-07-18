#ifndef LIBDRAGON_TOOLS_POLYFILL_H
#define LIBDRAGON_TOOLS_POLYFILL_H

#ifdef __MINGW32__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <share.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


// if typedef doesn't exist (msvc, blah)
typedef intptr_t ssize_t;

/* Fetched from: https://stackoverflow.com/a/47229318 */
/* The original code is public domain -- Will Hartung 4/9/09 */
/* Modifications, public domain as well, by Antti Haapala, 11/10/17
   - Switched to getc on 5/23/19 */

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = getc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = getc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

/* This function is original code in libdragon */
char *strndup(const char *s, size_t n)
{
  size_t len = strnlen(s, n);
  char *ret = malloc(len + 1);
  if (!ret) return NULL;
  memcpy(ret, s, len);
  ret[len] = '\0';
  return ret;
}

// tmpfile in mingw is broken (it uses msvcrt that tries to
// create a file in C:\, which is non-writable nowadays)
#define tmpfile()   mingw_tmpfile()

FILE *mingw_tmpfile(void) {
    // We use the current directory for temporary files. Using GetTempFilePath is dangerous
    // because a subprocess spawned without environment would receive C:\Windows which is not writable.
    // So the cwd has a higher chance of actually working, for our use case of command line tools. 
    char path[_MAX_PATH];
    for (int i=0; i<4096; i++) {
        // We use rand() which provides a 16-bit deterministic sequence. Again, for our use
        // case is sufficient, given that _O_EXCL will make sure the file does not exist.
        snprintf(path, sizeof(path), "mksprite-%04x", rand());
        // This is taken from mingw's mkstemp implementation, adding _O_TEMPORARY
        // to make the file autodelete
        int fd = _sopen(path, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY | _O_TEMPORARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
        if (fd != -1)
            return fdopen(fd, "w+b");
        if (fd == -1 && errno != EEXIST)
            return NULL;
    }
    return NULL;
}

#endif

#endif
