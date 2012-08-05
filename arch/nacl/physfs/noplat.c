/*
 * Posix-esque support routines for PhysicsFS.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#define __PHYSICSFS_INTERNAL__
#include "physfs_platforms.h"

#ifdef PHYSFS_PLATFORM_POSIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#ifdef PHYSFS_HAVE_LLSEEK
#include <linux/unistd.h>
#endif

#include "physfs_internal.h"
//#include "AS3.h"


const char *__PHYSFS_platformDirSeparator = "/";


char *__PHYSFS_platformCopyEnvironmentVariable(const char *varname)
{
    const char *envr = getenv(varname);
    char *retval = NULL;

    if (envr != NULL)
    {
        retval = (char *) allocator.Malloc(strlen(envr) + 1);
        if (retval != NULL)
            strcpy(retval, envr);
    } /* if */

    return(retval);
} /* __PHYSFS_platformCopyEnvironmentVariable */


static char *getUserNameByUID(void)
{
    uid_t uid = getuid();
    struct passwd *pw;
    char *retval = NULL;

    pw = getpwuid(uid);
    if ((pw != NULL) && (pw->pw_name != NULL))
    {
        retval = (char *) allocator.Malloc(strlen(pw->pw_name) + 1);
        if (retval != NULL)
            strcpy(retval, pw->pw_name);
    } /* if */
    
    return(retval);
} /* getUserNameByUID */


static char *getUserDirByUID(void)
{
    uid_t uid = getuid();
    struct passwd *pw;
    char *retval = NULL;

    pw = getpwuid(uid);
    if ((pw != NULL) && (pw->pw_dir != NULL))
    {
        retval = (char *) allocator.Malloc(strlen(pw->pw_dir) + 1);
        if (retval != NULL)
            strcpy(retval, pw->pw_dir);
    } /* if */
    
    return(retval);
} /* getUserDirByUID */


char *__PHYSFS_platformGetUserName(void)
{
    char *retval = getUserNameByUID();
    if (retval == NULL)
        retval = __PHYSFS_platformCopyEnvironmentVariable("USER");
    return(retval);
} /* __PHYSFS_platformGetUserName */


char *__PHYSFS_platformGetUserDir(void)
{
    char *retval = __PHYSFS_platformCopyEnvironmentVariable("HOME");
    if (retval == NULL)
        retval = getUserDirByUID();
    return(retval);
} /* __PHYSFS_platformGetUserDir */


#if 0
static AS3_Val descentHogVal, descentPigVal;

void setDescentHogVal(AS3_Val x) {
    if (descentHogVal)
        AS3_Release(descentHogVal);
    descentHogVal = x;
    AS3_Acquire(descentHogVal);
}

void setDescentPigVal(AS3_Val x) {
    if (descentPigVal)
        AS3_Release(descentPigVal);
    descentPigVal = x;
    AS3_Acquire(descentPigVal);
}

static AS3_Val getFileVal(const char *filename) {
    const char *p;
    sztrace("getFileVal");
    sztrace(filename);
    if ((p = strrchr(filename, '/')))
        p++;
    else
        p = filename;
    sztrace(p);
    if (!strcmp(p, "descent.hog"))
        return descentHogVal;
    if (!strcmp(p, "descent.pig"))
        return descentPigVal;
    return (AS3_Val)0;
}
#endif
static void *getFileVal(const char *filename) {
    return NULL;
}

static char *ememrchr(const char *buf, char s, int len) {
    const char *p = buf + len;
    while (p > buf)
        if (*--p == s)
            return (char *)p;
    return NULL;
}

static int ememnncmp(const char *a, int alen, const char *b, int blen) {
    int d = alen - blen;
    if (!d)
        d = memcmp(a, b, alen);
    return d;
}

static int isDir(const char *filename) {
    const char *p, *end = filename + strlen(filename);
    if (end > filename && end[-1] == '/')
        end--;
    if ((p = ememrchr(filename, '/', end - filename)))
        p++;
    else
        p = filename;
    return !ememnncmp(p, end - p, ".d1x-rebirth", strlen(".d1x-rebirth"));
}

int __PHYSFS_platformExists(const char *fname)
{
    BAIL_IF_MACRO(!isDir(fname) && !getFileVal(fname), strerror(ENOENT), 0);
    return(1);
} /* __PHYSFS_platformExists */


int __PHYSFS_platformIsSymLink(const char *fname)
{
    struct stat statbuf;
    BAIL_IF_MACRO(lstat(fname, &statbuf) == -1, strerror(errno), 0);
    return( (S_ISLNK(statbuf.st_mode)) ? 1 : 0 );
} /* __PHYSFS_platformIsSymlink */


int __PHYSFS_platformIsDirectory(const char *fname)
{
    //struct stat statbuf;
    //BAIL_IF_MACRO(stat(fname, &statbuf) == -1, strerror(errno), 0);
    //return( (S_ISDIR(statbuf.st_mode)) ? 1 : 0 );
    return isDir(fname) ? 1 : 0;
} /* __PHYSFS_platformIsDirectory */


char *__PHYSFS_platformCvtToDependent(const char *prepend,
                                      const char *dirName,
                                      const char *append)
{
    int len = ((prepend) ? strlen(prepend) : 0) +
              ((append) ? strlen(append) : 0) +
              strlen(dirName) + 1;
    char *retval = (char *) allocator.Malloc(len);

    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    /* platform-independent notation is Unix-style already.  :)  */

    if (prepend)
        strcpy(retval, prepend);
    else
        retval[0] = '\0';

    strcat(retval, dirName);

    if (append)
        strcat(retval, append);

    return(retval);
} /* __PHYSFS_platformCvtToDependent */



void __PHYSFS_platformEnumerateFiles(const char *dirname,
                                     int omitSymLinks,
                                     PHYSFS_EnumFilesCallback callback,
                                     const char *origdir,
                                     void *callbackdata)
{
    DIR *dir;
    struct dirent *ent;
    int bufsize = 0;
    char *buf = NULL;
    int dlen = 0;

    if (omitSymLinks)  /* !!! FIXME: this malloc sucks. */
    {
        dlen = strlen(dirname);
        bufsize = dlen + 256;
        buf = (char *) allocator.Malloc(bufsize);
        if (buf == NULL)
            return;
        strcpy(buf, dirname);
        if (buf[dlen - 1] != '/')
        {
            buf[dlen++] = '/';
            buf[dlen] = '\0';
        } /* if */
    } /* if */

    errno = 0;
    dir = opendir(dirname);
    if (dir == NULL)
    {
        allocator.Free(buf);
        return;
    } /* if */

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0)
            continue;

        if (strcmp(ent->d_name, "..") == 0)
            continue;

        if (omitSymLinks)
        {
            char *p;
            int len = strlen(ent->d_name) + dlen + 1;
            if (len > bufsize)
            {
                p = (char *) allocator.Realloc(buf, len);
                if (p == NULL)
                    continue;
                buf = p;
                bufsize = len;
            } /* if */

            strcpy(buf + dlen, ent->d_name);
            if (__PHYSFS_platformIsSymLink(buf))
                continue;
        } /* if */

        callback(callbackdata, origdir, ent->d_name);
    } /* while */

    allocator.Free(buf);
    closedir(dir);
} /* __PHYSFS_platformEnumerateFiles */


int __PHYSFS_platformMkDir(const char *path)
{
    int rc;
    errno = 0;
    rc = mkdir(path, S_IRWXU);
    BAIL_IF_MACRO(rc == -1, strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformMkDir */

#if 0
static int baRead(void *f, char *buf, int len) {
    AS3_Val ba = (AS3_Val)f;
    //char msgbuf[32];
    int ret = AS3_ByteArray_readBytes(buf, ba, len);
    //sprintf(msgbuf, "baRead %d = %d", len, ret);
    //sztrace(msgbuf);
    return ret;
}

static int baWrite(void *f, const char *buf, int len) {
    AS3_Val ba = (AS3_Val)f;
    int ret = AS3_ByteArray_writeBytes(ba, (char *)buf, len);
    return ret;
}

static fpos_t baSeek(void *f, fpos_t pos, int whence) {
    AS3_Val ba = (AS3_Val)f;
    return AS3_ByteArray_seek(ba, pos, whence);
}

static int baClose(void *f) {
    return 0;
}

static void *doOpen(const char *filename, int mode)
{
    //const int appending = (mode & O_APPEND);
    AS3_Val fileVal;

    /* O_APPEND doesn't actually behave as we'd like. */
    //mode &= ~O_APPEND;

    fileVal = getFileVal(filename);
    if (!fileVal) {
        BAIL_IF_MACRO(1, strerror(ENOENT), NULL);
    }

    AS3_ByteArray_seek(fileVal, 0, SEEK_SET);

    return funopen(fileVal, baRead, baWrite, baSeek, baClose);
} /* doOpen */
#endif
static void *doOpen(const char *filename, int mode) {
    return NULL;
}

void *__PHYSFS_platformOpenRead(const char *filename)
{
    return(doOpen(filename, O_RDONLY));
} /* __PHYSFS_platformOpenRead */


void *__PHYSFS_platformOpenWrite(const char *filename)
{
    return(doOpen(filename, O_WRONLY | O_CREAT | O_TRUNC));
} /* __PHYSFS_platformOpenWrite */


void *__PHYSFS_platformOpenAppend(const char *filename)
{
    return(doOpen(filename, O_WRONLY | O_CREAT | O_APPEND));
} /* __PHYSFS_platformOpenAppend */


PHYSFS_sint64 __PHYSFS_platformRead(void *opaque, void *buffer,
                                    PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    FILE *f = opaque;
    size_t rc = fread(buffer, size, count, f);

    BAIL_IF_MACRO(rc < count, strerror(errno), rc);

    return rc;
} /* __PHYSFS_platformRead */


PHYSFS_sint64 __PHYSFS_platformWrite(void *opaque, const void *buffer,
                                     PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    FILE *f = opaque;
    size_t rc = fwrite(buffer, size, count, f);

    BAIL_IF_MACRO(rc < count, strerror(errno), rc);

    return rc;
} /* __PHYSFS_platformWrite */


int __PHYSFS_platformSeek(void *opaque, PHYSFS_uint64 pos)
{
    FILE *f = opaque;

    BAIL_IF_MACRO(fseek(f, (long) pos, SEEK_SET) == -1, strerror(errno), 0);

    return(1);
} /* __PHYSFS_platformSeek */


PHYSFS_sint64 __PHYSFS_platformTell(void *opaque)
{
    FILE *f = opaque;
    return ftell(f);
} /* __PHYSFS_platformTell */


PHYSFS_sint64 __PHYSFS_platformFileLength(void *opaque)
{
    FILE *f = opaque;
    long pos, len;
    pos = ftell(f);
    BAIL_IF_MACRO(fseek(f, 0, SEEK_END), strerror(errno), -1);
    len = ftell(f);
    BAIL_IF_MACRO(fseek(f, pos, SEEK_SET), strerror(errno), -1);
    return len;
} /* __PHYSFS_platformFileLength */


int __PHYSFS_platformEOF(void *opaque)
{
    FILE *f = opaque;
    return feof(f);
} /* __PHYSFS_platformEOF */


int __PHYSFS_platformFlush(void *opaque)
{
    FILE *f = opaque;
    BAIL_IF_MACRO(fflush(f), strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformFlush */


int __PHYSFS_platformClose(void *opaque)
{
    FILE *f = opaque;
    BAIL_IF_MACRO(fclose(f), strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformClose */


int __PHYSFS_platformDelete(const char *path)
{
    BAIL_IF_MACRO(remove(path) == -1, strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformDelete */


PHYSFS_sint64 __PHYSFS_platformGetLastModTime(const char *fname)
{
    struct stat statbuf;
    BAIL_IF_MACRO(stat(fname, &statbuf) < 0, strerror(errno), -1);
    return statbuf.st_mtime;
} /* __PHYSFS_platformGetLastModTime */

#endif  /* PHYSFS_PLATFORM_POSIX */

/* end of posix.c ... */

