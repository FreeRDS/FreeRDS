/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
 * Copyright 2013 Laxmikant Rashinkar <LK.Rashinkar@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RDPDR_FUSE_H
#define _RDPDR_FUSE_H

#include <winpr/wtypes.h>

#include "rdpdr_plugin.h"

/* a file or dir entry in the FreeRDS file system */
struct rdpdr_inode
{
    UINT32          parent_inode;      /* Parent serial number.             */
    UINT32          inode;             /* File serial number.               */
    UINT32          mode;              /* File mode.                        */
    UINT32          nlink;             /* symbolic link count.              */
    UINT32          nentries;          /* number of entries in a dir        */
    UINT32          nopen;             /* number of simultaneous opens      */
    UINT32          uid;               /* User ID of the file's owner.      */
    UINT32          gid;               /* Group ID of the file's group.     */
    size_t          size;              /* Size of file, in bytes.           */
    time_t          atime;             /* Time of last access.              */
    time_t          mtime;             /* Time of last modification.        */
    time_t          ctime;             /* Time of last status change.       */
    char            name[256];         /* Dir or filename                   */
    UINT32          device_id;         /* for file system redirection       */
    char            is_synced;         /* dir struct has been read from     */
                                       /* remote device, done just once     */
    int             lindex;            /* used in clipboard operations      */
    int             is_loc_resource;   /* this is not a redirected resource */
    int             close_in_progress; /* close cmd sent to client          */
    int             stale;             /* mark file as stale, ok to remove  */
};
typedef struct rdpdr_inode RDPDR_INODE;

int rdpdr_fuse_init(RdpdrPluginContext *rdpdrPluginContext);
int rdpdr_fuse_deinit();
int rdpdr_fuse_check_wait_objs(void);
int rdpdr_fuse_get_wait_objs(int *objs, int *count, int *timeout);

int rdpdr_fuse_clear_clip_dir(void);
int rdpdr_fuse_file_contents_range(int stream_id, char *data, int data_bytes);
int rdpdr_fuse_file_contents_size(int stream_id, int file_size);
int rdpdr_fuse_add_clip_dir_item(char *filename, int flags, int size, int lindex);

#endif
