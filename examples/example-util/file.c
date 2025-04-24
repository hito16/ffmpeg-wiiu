#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "exutil.h"

/*  Check in sd:/media for the first non-directory and fill the buffer
    with the fully qualified path + filename.
    return 0 on success, <0 for errors
*/
int util_get_first_media_file(char *buffer, int size) {
    /* try to find /media in the WIIU SD mount, then the CEMU SD mount */
    char *sdmounts[] = {
        "/vol/external01/media",
        "/vol/storage_mlc01/media",
    };
    int sd_idx = (access(sdmounts[0], F_OK) == 0)   ? 0
                 : (access(sdmounts[1], F_OK) == 0) ? 1
                                                    : -1;
    if (sd_idx < 0) {
        return -1;
    }

    DIR *dp = NULL;
    struct dirent *ep = NULL;

    dp = opendir(sdmounts[sd_idx]);
    if (dp != NULL) {
        while (ep = readdir(dp)) {
            if (ep->d_type == DT_REG) {
                snprintf(buffer, size, "%s/%s", sdmounts[sd_idx], ep->d_name);
                return 0;
            }
        }
        closedir(dp);
    }
    return -2;
}

int64_t util_get_file_size(char *filename) {
    struct stat f_stat;
    if (stat(filename, &f_stat) < 0) {
        return -1;
    }
    return f_stat.st_size;
}