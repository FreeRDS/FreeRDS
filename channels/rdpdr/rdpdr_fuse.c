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

/*
 * TODO
 *      o when creating dir/file, ensure it does not already exist
 *      o do not allow dirs to be created in ino==1 except for .clipbard and share mounts
 *      o fix the HACK where I have to use my own buf instead of g_buffer
 *        this is in func xfuse_check_wait_objs()
 *      o if fuse mount point is already mounted, I get segfault
 *      o in open, check for modes such as O_TRUNC, O_APPEND
 *      o copying over an existing file does not work
 *      o after a dir is created, the device cannot be unmounted on the client side
 *        so something is holding it up
 *      o in thunar, when I move a file by dragging to another folder, the file
 *        is getting copied instead of being moved
 *      o unable to edit files in vi
 *      o fuse ops to support
 *          o touch does not work
 *          o chmod must work
 *          o cat >> file is not working
 *
 */

//#define USE_SYNC_FLAG

#define RDPDR_FUSE

#ifndef RDPDR_FUSE

/******************************************************************************
**                                                                           **
**                  when FUSE is NOT enabled in rdpdr                        **
**                                                                           **
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdpdr_fuse.h"

/* dummy calls when RDPDR_FUSE is not defined */
int rdpdr_fuse_init(RdpdrPluginContext *rdpdrPluginContext) { return 0; }
int rdpdr_fuse_deinit() { return 0; }
int rdpdr_fuse_check_wait_objs(void) { return 0; }
int rdpdr_fuse_get_wait_objs(HANDLE *objs, int *count, int *timeout) { return 0; }
int rdpdr_fuse_clear_clip_dir(void)  { return 0; }
int rdpdr_fuse_file_contents_range(int stream_id, char *data, int data_bytes) { return 0; }
int rdpdr_fuse_file_contents_size(int stream_id, int file_size) { return 0; }
int rdpdr_fuse_add_clip_dir_item(char *filename, int flags, int size, int lindex) { return 0; }

#else

/******************************************************************************
**                                                                           **
**                    when FUSE is enabled in rdpdr                          **
**                                                                           **
******************************************************************************/

#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64
#include <fuse/fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <sched.h>

#include <winpr/collections.h>
#include <winpr/wlog.h>
#include <winpr/wtypes.h>

#include "rdpdr_fuse.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

#define RDPDR_FUSE_ATTR_TIMEOUT      1.0
#define RDPDR_FUSE_ENTRY_TIMEOUT     1.0

#define DOTDOT_INODE    0
#define DOT_INODE       0
#define FIRST_INODE     1

#define OP_RENAME_FILE  0x01

#define WINDOWS_TO_LINUX_FILE_PERM(_a) \
            (((_a) & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR | 0100 : S_IFREG) |\
            (((_a) & FILE_ATTRIBUTE_READONLY)  ? 0444 : 0644)

/* winodws time starts on Jan 1, 1601 */
/* Linux   time starts on Jan 1, 1970 */
#define EPOCH_DIFF 11644473600LL
#define WINDOWS_TO_LINUX_TIME(_t) ((_t) / 10000000) - EPOCH_DIFF;

/* FUSE mount point */
static char g_fuse_root_path[256] = "";
static char g_fuse_clipboard_path[256] = ""; /* for clipboard use */

static wLog *g_logger;

static RdpdrPluginContext *g_rdpdrPluginContext;
static RdpdrServerContext *g_rdpdrServerContext;


/* the rdpdr file system in memory */
struct rdpdr_fs
{
    RDPDR_INODE **inode_table;  /* a table of entries; can grow         */
    unsigned int max_entries;   /* size of inode_table[]                */
    unsigned int num_entries;   /* num entries available in inode_table */
    unsigned int next_node;     /* next free node number                */
};

struct dirbuf
{
    char *p;
    size_t size;
};

struct dirbuf1
{
    char buf[4096];
    int  bytes_in_buf;
    int  first_time;
};

/* FUSE reply types */
#define RT_FUSE_REPLY_OPEN      1
#define RT_FUSE_REPLY_CREATE    2

struct rdpdr_fuse_info
{
    struct fuse_file_info *fi;
    fuse_req_t             req;
    fuse_ino_t             inode;
    fuse_ino_t             new_inode;
    int                    invoke_fuse;
    char                   name[1024];
    char                   new_name[1024];
    UINT32                 device_id;
    int                    reply_type;
    int                    mode;
    int                    type;
    size_t                 size;
    off_t                  off;
    struct dirbuf1         dirbuf1;
};
typedef struct rdpdr_fuse_info RDPDR_FUSE_INFO;

struct rdpdr_fuse_handle
{
    UINT32 DeviceId;
    UINT32 FileId;
    int    is_loc_resource; /* this is not a redirected resource */
};
typedef struct rdpdr_fuse_handle RDPDR_FUSE_HANDLE;

/* used for file data request sent to client */
struct req_list_item
{
    fuse_req_t req;
    int stream_id;
    int lindex;
    int off;
    int size;
};

struct dir_info
{
    /* last index accessed in g_rdpdr_fs.inode_table[] */
    int index;
};

/* queue FUSE opendir commands so we run only one at a time */
struct opendir_req
{
    fuse_req_t             req;
    fuse_ino_t             ino;
    struct fuse_file_info *fi;
};

static wQueue *g_opendir_queue;

static wArrayList *g_req_list;
static struct rdpdr_fs g_rdpdr_fs;                /* an inst of RDPDR file system */
static char *g_mount_point = 0;                   /* our FUSE mount point        */
static struct fuse_lowlevel_ops g_rdpdr_fuse_ops; /* setup FUSE callbacks        */
static int g_rdpdr_fuse_inited = 0;               /* true when FUSE is inited    */
static struct fuse_chan *g_ch = 0;
static struct fuse_session *g_se = 0;
static char *g_buffer = 0;
static int g_fd = 0;
static LONG_PTR g_bufsize = 0;

/* forward declarations for internal access */
static int rdpdr_fuse_init_fs();
static int rdpdr_fuse_deinit_fs();
static int rdpdr_fuse_init_lib(struct fuse_args *args);
static int rdpdr_fuse_is_inode_valid(int ino);

// LK_TODO
#if 0
static void xfuse_create_file(fuse_req_t req, fuse_ino_t parent,
                              const char *name, mode_t mode, int type);
#endif

static void rdpdr_fuse_dump_fs();
static void rdpdr_fuse_dump_inode(RDPDR_INODE *rinode);
static UINT32 rdpdr_fuse_get_device_id_for_inode(UINT32 ino, char *full_path);
static void fuse_reverse_pathname(char *full_path, char *reverse_path);

static RDPDR_INODE *rdpdr_fuse_get_rinode_from_pinode_name(UINT32 pinode,
                                                            const char *name);

static RDPDR_INODE *rdpdr_fuse_create_file_in_fs(UINT32 device_id,
                                                        int pinode, char *name,
                                                        int type);

static int  rdpdr_fuse_does_file_exist(int parent, char *name);
static int  rdpdr_fuse_delete_file(int parent, char *name);
static int  rdpdr_fuse_delete_file_with_rinode(RDPDR_INODE *rinode);
static int  rdpdr_fuse_delete_dir_with_rinode(RDPDR_INODE *rinode);
static int  rdpdr_fuse_recursive_delete_dir_with_rinode(RDPDR_INODE *rinode);
static void rdpdr_fuse_update_fs_size();
static void rdpdr_fuse_enum_dir(fuse_req_t req, fuse_ino_t ino, size_t size,
                           off_t off, struct fuse_file_info *fi);

static void rdpdr_fuse_remove_dir_or_file(fuse_req_t req, fuse_ino_t parent,
                                     const char *name, int type);

static void rdpdr_fuse_create_dir_or_file(fuse_req_t req, fuse_ino_t parent,
                                     const char *name, mode_t mode,
                                     struct fuse_file_info *fi, int type);

/* forward declarations for FUSE callbacks */
static void rdpdr_fuse_cb_lookup(fuse_req_t req, fuse_ino_t parent,
                            const char *name);

static void rdpdr_fuse_cb_getattr(fuse_req_t req, fuse_ino_t ino,
                             struct fuse_file_info *fi);

static void rdpdr_fuse_cb_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                             off_t off, struct fuse_file_info *fi);

static void rdpdr_fuse_cb_mkdir(fuse_req_t req, fuse_ino_t parent,
                           const char *name, mode_t mode);

static void rdpdr_fuse_cb_rmdir(fuse_req_t req, fuse_ino_t parent,
                           const char *name);

static void rdpdr_fuse_cb_unlink(fuse_req_t req, fuse_ino_t parent,
                            const char *name);

static void rdpdr_fuse_cb_rename(fuse_req_t req,
                            fuse_ino_t old_parent, const char *old_name,
                            fuse_ino_t new_parent, const char *new_name);

static void rdpdr_fuse_cb_open(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi);

static void rdpdr_fuse_cb_release(fuse_req_t req, fuse_ino_t ino, struct
                             fuse_file_info *fi);

static void rdpdr_fuse_cb_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, struct fuse_file_info *fi);

static void rdpdr_fuse_cb_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                           size_t size, off_t off, struct fuse_file_info *fi);

static void rdpdr_fuse_cb_create(fuse_req_t req, fuse_ino_t parent,
                            const char *name, mode_t mode,
                            struct fuse_file_info *fi);

static void rdpdr_fuse_cb_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                           struct fuse_file_info *fi);

static void rdpdr_fuse_cb_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                             int to_set, struct fuse_file_info *fi);

static void rdpdr_fuse_cb_opendir(fuse_req_t req, fuse_ino_t ino,
                             struct fuse_file_info *fi);

static int rdpdr_fuse_proc_opendir_req(fuse_req_t req, fuse_ino_t ino,
                                  struct fuse_file_info *fi);

static void rdpdr_fuse_cb_releasedir(fuse_req_t req, fuse_ino_t ino,
                                struct fuse_file_info *fi);

/* forward declarations for RDPDR server callbacks */
static void rdpdr_fuse_on_drive_create(RdpdrServerContext* serverContext, UINT32 deviceId, const char* name);
static void rdpdr_fuse_on_drive_delete(RdpdrServerContext* serverContext, UINT32 deviceId);
static void rdpdr_fuse_on_drive_create_directory_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus);
static void rdpdr_fuse_on_drive_delete_directory_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus);
static void rdpdr_fuse_on_drive_query_directory_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus, FILE_DIRECTORY_INFORMATION* fdi);
static void rdpdr_fuse_on_drive_open_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus, UINT32 deviceId, UINT32 fileId);
static void rdpdr_fuse_on_drive_read_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus, const char* buffer, UINT32 length);
static void rdpdr_fuse_on_drive_write_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus, UINT32 bytesWritten);
static void rdpdr_fuse_on_drive_close_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus);
static void rdpdr_fuse_on_drive_delete_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus);
static void rdpdr_fuse_on_drive_rename_file_complete(RdpdrServerContext* serverContext, void* callbackData, UINT32 ioStatus);

/* clipboard calls */
int clipboard_request_file_data(int stream_id, int lindex, int offset,
                                int request_bytes);

/* misc calls */
static void rdpdr_fuse_mark_as_stale(int pinode);
static void rdpdr_fuse_delete_stale_entries(int pinode);

/*****************************************************************************
**                                                                          **
**         public functions - can be called from any code path              **
**                                                                          **
*****************************************************************************/

/**
 * Initialize FUSE subsystem
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_init(RdpdrPluginContext *rdpdrPluginContext)
{
    struct stat statbuf;

    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

    g_rdpdrPluginContext = rdpdrPluginContext;
	g_rdpdrServerContext = rdpdrPluginContext->rdpdrServer;

    g_logger = g_rdpdrPluginContext->log;

    /* if already inited, just return */
    if (g_rdpdr_fuse_inited)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "already inited");
        return 1;
    }

    if (g_ch != 0)
    {
        WLog_Print(g_logger, WLOG_ERROR, "g_ch is not zero");
        return -1;
    }

    /* define FUSE mount point to ~/freerds_client */
    WLog_Print(g_logger, WLOG_DEBUG, "defining fuse mount point");
    snprintf(g_fuse_root_path, 255, "%s/freerds_client", getenv("HOME"));
    snprintf(g_fuse_clipboard_path, 255, "%s/.clipboard", g_fuse_root_path);

    /* if FUSE mount point does not exist, create it */
    WLog_Print(g_logger, WLOG_DEBUG, "checking fuse mount point %s", g_fuse_root_path);
    if ((stat(g_fuse_root_path, &statbuf) != 0) || !S_ISDIR(statbuf.st_mode))
    {
		WLog_Print(g_logger, WLOG_DEBUG, "creating fuse mount point %s", g_fuse_root_path);
        if (mkdir(g_fuse_root_path, 0777) != 0)
        {
            WLog_Print(g_logger, WLOG_ERROR, "mkdir %s failed. If %s is already mounted, you must "
                      "first unmount it", g_fuse_root_path, g_fuse_root_path);
            return -1;
        }
    }

    /* setup file system */
    WLog_Print(g_logger, WLOG_DEBUG, "calling rdpdr_fuse_init_fs");
    if (rdpdr_fuse_init_fs())
        return -1;

    /* setup queues */
    g_opendir_queue = Queue_New(TRUE, 20, 10);

    /* setup FUSE callbacks */
    memset(&g_rdpdr_fuse_ops, 0, sizeof(g_rdpdr_fuse_ops));
    g_rdpdr_fuse_ops.lookup      = rdpdr_fuse_cb_lookup;
    g_rdpdr_fuse_ops.readdir     = rdpdr_fuse_cb_readdir;
    g_rdpdr_fuse_ops.mkdir       = rdpdr_fuse_cb_mkdir;
    g_rdpdr_fuse_ops.rmdir       = rdpdr_fuse_cb_rmdir;
    g_rdpdr_fuse_ops.unlink      = rdpdr_fuse_cb_unlink;
    g_rdpdr_fuse_ops.rename      = rdpdr_fuse_cb_rename;
    g_rdpdr_fuse_ops.open        = rdpdr_fuse_cb_open;
    g_rdpdr_fuse_ops.release     = rdpdr_fuse_cb_release;
    g_rdpdr_fuse_ops.read        = rdpdr_fuse_cb_read;
    g_rdpdr_fuse_ops.write       = rdpdr_fuse_cb_write;
    g_rdpdr_fuse_ops.create      = rdpdr_fuse_cb_create;
    //g_rdpdr_fuse_ops.fsync     = rdpdr_fuse_cb_fsync; /* LK_TODO delete this */
    g_rdpdr_fuse_ops.getattr     = rdpdr_fuse_cb_getattr;
    g_rdpdr_fuse_ops.setattr     = rdpdr_fuse_cb_setattr;
    g_rdpdr_fuse_ops.opendir     = rdpdr_fuse_cb_opendir;
    g_rdpdr_fuse_ops.releasedir  = rdpdr_fuse_cb_releasedir;

    fuse_opt_add_arg(&args, "freerds-channels");
    fuse_opt_add_arg(&args, g_fuse_root_path);
#if 0
    fuse_opt_add_arg(&args, "-s"); /* single threaded mode */
    fuse_opt_add_arg(&args, "-d"); /* debug mode           */
#endif

    if (rdpdr_fuse_init_lib(&args))
    {
        rdpdr_fuse_deinit();
        return -1;
    }

	/* setup RDPDR server callbacks */
	g_rdpdrServerContext->OnDriveCreate = rdpdr_fuse_on_drive_create;
	g_rdpdrServerContext->OnDriveDelete = rdpdr_fuse_on_drive_delete;
	g_rdpdrServerContext->OnDriveCreateDirectoryComplete = rdpdr_fuse_on_drive_create_directory_complete;
	g_rdpdrServerContext->OnDriveDeleteDirectoryComplete = rdpdr_fuse_on_drive_delete_directory_complete;
	g_rdpdrServerContext->OnDriveQueryDirectoryComplete = rdpdr_fuse_on_drive_query_directory_complete;
	g_rdpdrServerContext->OnDriveOpenFileComplete = rdpdr_fuse_on_drive_open_file_complete;
	g_rdpdrServerContext->OnDriveReadFileComplete = rdpdr_fuse_on_drive_read_file_complete;
	g_rdpdrServerContext->OnDriveWriteFileComplete = rdpdr_fuse_on_drive_write_file_complete;
	g_rdpdrServerContext->OnDriveCloseFileComplete = rdpdr_fuse_on_drive_close_file_complete;
	g_rdpdrServerContext->OnDriveDeleteFileComplete = rdpdr_fuse_on_drive_delete_file_complete;
	g_rdpdrServerContext->OnDriveRenameFileComplete = rdpdr_fuse_on_drive_rename_file_complete;

    g_rdpdr_fuse_inited = 1;

    return 0;
}

/**
 * De-initialize FUSE subsystem
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_deinit()
{
    rdpdr_fuse_deinit_fs();

	if (g_opendir_queue != NULL)
	{
		Queue_Free(g_opendir_queue);
		g_opendir_queue = NULL;
	}

    if (g_ch != 0)
    {
        fuse_session_remove_chan(g_ch);
        fuse_unmount(g_mount_point, g_ch);
        g_ch = 0;
    }

    if (g_se != 0)
    {
		fuse_remove_signal_handlers(g_se);
        fuse_session_destroy(g_se);
        g_se = 0;
    }

    if (g_buffer != 0)
    {
        free(g_buffer);
        g_buffer = 0;
    }

    if (g_req_list != 0)
    {
		ArrayList_Free(g_req_list);
        g_req_list = 0;
    }

    g_logger = NULL;

    g_rdpdrPluginContext = NULL;
    g_rdpdrServerContext = NULL;

    g_rdpdr_fuse_inited = 0;

    return 0;
}

/**
 *
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/
int rdpdr_fuse_check_wait_objs(void)
{
#if 0
    struct fuse_chan *tmpch;
    struct timeval    timeval;
    fd_set            rfds;
    int               rval;

#define HACK

#ifdef HACK
char buf[135168];
#endif

    if (g_ch == 0)
        return 0;

    FD_ZERO(&rfds);
    FD_SET(g_fd, &rfds);

    memset(&timeval, 0, sizeof(timeval));

    rval = select(g_fd + 1, &rfds, NULL, NULL, &timeval);

    if (rval > 0)
    {
        printf("rdpdr_fuse_check_wait_objs - rval=%d\n", rval); fflush(stdout);

        tmpch = g_ch;

#ifdef HACK
        rval = fuse_chan_recv(&tmpch, buf, g_bufsize);
#else
        rval = fuse_chan_recv(&tmpch, g_buffer, g_bufsize);
#endif
        if (rval == -EINTR)
            return -1;

        if (rval == -ENODEV)
            return -1;

        if (rval <= 0)
            return -1;

#ifdef HACK
        fuse_session_process(g_se, buf, rval, tmpch);
#else
        fuse_session_process(g_se, g_buffer, rval, tmpch);
#endif
    }
#else
	fuse_session_loop(g_se);
#endif

    return 0;
}

/**
 *
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_get_wait_objs(int *objs, int *count, int *timeout)
{
    int lcount;

    if (g_ch == 0)
        return 0;

    lcount = *count;
    objs[lcount] = g_fd;
    lcount++;
    *count = lcount;

    return 0;
}

/***************************************
 * RDPDR Drive Create
 **************************************/

static void rdpdr_fuse_on_drive_create(RdpdrServerContext* serverContext, UINT32 deviceId, const char* name)
{
    //RDPDR_FUSE_INFO *fip;
    RDPDR_INODE *rinode;
	char dirname[2];

	WLog_Print(g_logger, WLOG_DEBUG, "on_drive_create: deviceId=%u, name=%s", deviceId, name);

	if (name == NULL || (strlen(name) == 0))
	{
		WLog_Print(g_logger, WLOG_ERROR, "invalid drive name");
		return;
	}

	dirname[0] = name[0];
	dirname[1] = '\0';

    if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "calloc() failed");
        return;
    }

    /* create directory entry */
    rinode->parent_inode = 1;
    rinode->inode = g_rdpdr_fs.next_node++;
    rinode->mode = 0755 | S_IFDIR;
    rinode->nlink = 1;
    rinode->uid = getuid();
    rinode->gid = getgid();
    rinode->size = 0;
    rinode->atime = time(0);
    rinode->mtime = time(0);
    rinode->ctime = time(0);
    strcpy(rinode->name, dirname);
    rinode->device_id = deviceId;

    g_rdpdr_fs.num_entries++;
    /* saved_inode = rinode->inode; */

    /* insert it in file system */
    g_rdpdr_fs.inode_table[rinode->inode] = rinode;
    WLog_Print(g_logger, WLOG_DEBUG, "created new share named %s at inode_table[%d]",
              dirname, (int) rinode->inode);

    /* update nentries in parent inode */
    rinode = g_rdpdr_fs.inode_table[1];
    if (rinode == NULL)
        return;
    rinode->nentries++;

#if 0
    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        return -1;
    }

     /* enumerate root dir, do not call FUSE when done */
    fip->req = NULL;
    fip->inode = 1; /* TODO saved_inode; */
    strncpy(fip->name, dirname, 1024);
    fip->name[1023] = 0;
    fip->device_id = device_id;

    g_rdpdrServerContext->DriveQueryDirectory(g_rdpdrServerContext, (void *) fip, device_id, "\\");
#endif
}

/***************************************
 * RDPDR Drive Delete
 **************************************/

static void rdpdr_fuse_on_drive_delete(RdpdrServerContext* serverContext, UINT32 deviceId)
{
	WLog_Print(g_logger, WLOG_DEBUG, "on_drive_delete: deviceId=%u", deviceId);
}

/**
 * Clear all clipboard entries in file system
 *
 * This function is called by clipboard code
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_clear_clip_dir(void)
{
    int          i;
    RDPDR_INODE *rinode;
    RDPDR_INODE *rip;

    WLog_Print(g_logger, WLOG_DEBUG, "entered");

    /* xinode for .clipboard */
    rip = g_rdpdr_fs.inode_table[2];

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        if (rinode->parent_inode == 2)
        {
            g_rdpdr_fs.inode_table[i] = NULL;
            free(rinode);
            rip->nentries--;
        }
    }

    return 0;
}

/**
 * Return clipboard data to fuse
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_file_contents_range(int stream_id, char *data, int data_bytes)
{
    WLog_Print(g_logger, WLOG_DEBUG, "entered: stream_id=%d data_bytes=%d", stream_id, data_bytes);

    struct req_list_item *rli;

    if ((rli = (struct req_list_item *) ArrayList_GetItem(g_req_list, 0)) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "range error!");
        return -1;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "lindex=%d off=%d size=%d", rli->lindex, rli->off, rli->size);

    fuse_reply_buf(rli->req, data, data_bytes);

    ArrayList_RemoveAt(g_req_list, 0);
    if (ArrayList_Count(g_req_list) <= 0)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "completed all requests");
        return 0;
    }

    /* send next request */
    rli = (struct req_list_item *) ArrayList_GetItem(g_req_list, 0);
    if (rli == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "range error!");
        return -1;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "requesting clipboard file data");

    clipboard_request_file_data(rli->stream_id, rli->lindex,
                                    rli->off, rli->size);

    return 0;
}

/**
 * Create a file in .clipboard dir
 *
 * This function is called by clipboard code
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_add_clip_dir_item(char *filename, int flags, int size, int lindex)
{
    WLog_Print(g_logger, WLOG_DEBUG, "entered: filename=%s flags=%d size=%d lindex=%d",
              filename, flags, size, lindex);

    /* add entry to file system */
    RDPDR_INODE *rinode = rdpdr_fuse_create_file_in_fs(0,    /* device id    */
                                                       2,    /* parent inode */
                                                       filename,
                                                       S_IFREG);
    rinode->size = size;
    rinode->lindex = lindex;
    rinode->is_loc_resource = 1;

    return 0;
}

/**
 *
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

int rdpdr_fuse_file_contents_size(int stream_id, int file_size)
{
    WLog_Print(g_logger, WLOG_DEBUG, "entered: stream_id=%d file_size=%d", stream_id, file_size);
    return 0;
}

/*****************************************************************************
**                                                                          **
**       private functions - can only be called from within this file       **
**                                                                          **
*****************************************************************************/

/**
 * Initialize FUSE library
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

static int rdpdr_fuse_init_lib(struct fuse_args *args)
{
    if (fuse_parse_cmdline(args, &g_mount_point, 0, 0) < 0)
    {
        WLog_Print(g_logger, WLOG_ERROR, "fuse_parse_cmdline() failed");
        fuse_opt_free_args(args);
        return -1;
    }

    if ((g_ch = fuse_mount(g_mount_point, args)) == 0)
    {
        WLog_Print(g_logger, WLOG_ERROR, "fuse_mount() failed");
        fuse_opt_free_args(args);
        return -1;
    }

    g_se = fuse_lowlevel_new(args, &g_rdpdr_fuse_ops, sizeof(g_rdpdr_fuse_ops), 0);
    if (g_se == 0)
    {
        WLog_Print(g_logger, WLOG_ERROR, "fuse_lowlevel_new() failed");
        fuse_unmount(g_mount_point, g_ch);
        g_ch = 0;
        fuse_opt_free_args(args);
        return -1;
    }

    fuse_opt_free_args(args);
	fuse_set_signal_handlers(g_se);
    fuse_session_add_chan(g_se, g_ch);
    g_bufsize = fuse_chan_bufsize(g_ch);

    g_buffer = calloc(g_bufsize, 1);
    g_fd = fuse_chan_fd(g_ch);

    g_req_list = ArrayList_New(TRUE);

    return 0;
}

/**
 * Initialize file system
 *
 * @return 0 on success, -1 on failure
 *
 *****************************************************************************/

static int rdpdr_fuse_init_fs()
{
    RDPDR_INODE *rinode;

    g_rdpdr_fs.inode_table = calloc(4096, sizeof(RDPDR_INODE *));
    if (g_rdpdr_fs.inode_table == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        return -1;
    }

    /*
     * index 0 is our .. dir
     */

    if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        free(g_rdpdr_fs.inode_table);
        return -1;
    }
    g_rdpdr_fs.inode_table[0] = rinode;
    rinode->parent_inode = 0;
    rinode->inode = 0;
    rinode->mode = S_IFDIR | 0755;
    rinode->nentries = 1;
    rinode->uid = getuid();
    rinode->gid = getgid();
    rinode->size = 0;
    rinode->atime = time(0);
    rinode->mtime = time(0);
    rinode->ctime = time(0);
    strcpy(rinode->name, "..");

    /*
     * index 1 is our . dir
     */

    if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        free(g_rdpdr_fs.inode_table[0]);
        free(g_rdpdr_fs.inode_table);
        return -1;
    }
    g_rdpdr_fs.inode_table[1] = rinode;
    rinode->parent_inode = 0;
    rinode->inode = 1;
    rinode->mode = S_IFDIR | 0755;
    rinode->nentries = 1;
    rinode->uid = getuid();
    rinode->gid = getgid();
    rinode->size = 0;
    rinode->atime = time(0);
    rinode->mtime = time(0);
    rinode->ctime = time(0);
    strcpy(rinode->name, ".");

    /*
     * index 2 is for clipboard use
     */

    if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        free(g_rdpdr_fs.inode_table[0]);
        free(g_rdpdr_fs.inode_table[1]);
        free(g_rdpdr_fs.inode_table);
        return -1;
    }

    g_rdpdr_fs.inode_table[2] = rinode;
    rinode->parent_inode = 1;
    rinode->inode = 2;
    rinode->nentries = 0;
    rinode->mode = S_IFDIR | 0755;
    rinode->uid = getuid();
    rinode->gid = getgid();
    rinode->size = 0;
    rinode->atime = time(0);
    rinode->mtime = time(0);
    rinode->ctime = time(0);
    rinode->is_loc_resource = 1;
    strcpy(rinode->name, ".clipboard");

    g_rdpdr_fs.max_entries = 4096;
    g_rdpdr_fs.num_entries = 3;
    g_rdpdr_fs.next_node = 3;

    return 0;
}

/**
 * zap the file system
 *
 * @return 0 on success, -1 on failure
 *****************************************************************************/

static int rdpdr_fuse_deinit_fs()
{
    return 0;
}

/**
 * determine if specified ino exists in file system
 *
 * @return 1 if it does, 0 otherwise
 *****************************************************************************/

static int rdpdr_fuse_is_inode_valid(int ino)
{
    /* is ino present in our table? */
    if ((ino < FIRST_INODE) || (ino >= g_rdpdr_fs.next_node))
        return 0;

    if (g_rdpdr_fs.inode_table[ino] == NULL)
        return 0;

    return 1;
}

/**
 * @brief Create a directory or regular file.
 *****************************************************************************/

// LK_TODO
#if 0
static void rdpr_fuse_create_file(fuse_req_t req, fuse_ino_t parent,
                              const char *name, mode_t mode, int type)
{
    struct xrdp_inode        *xinode;
    struct fuse_entry_param   e;

    log_debug("parent=%d name=%s", (int) parent, name);

    /* do we have a valid parent inode? */
    if (!xfuse_is_inode_valid(parent))
    {
        log_error("inode %d is not valid", parent);
        fuse_reply_err(req, EBADF);
    }

    if ((xinode = calloc(1, sizeof(struct xrdp_inode))) == NULL)
    {
        log_error("calloc() failed");
        fuse_reply_err(req, ENOMEM);
    }

    /* create directory entry */
    xinode->parent_inode = parent;
    xinode->inode = g_rdpdr_fs.next_node++; /* TODO should be thread safe */
    xinode->mode = mode | type;
    xinode->uid = getuid();
    xinode->gid = getgid();
    xinode->size = 0;
    xinode->atime = time(0);
    xinode->mtime = time(0);
    xinode->ctime = time(0);
    strcpy(xinode->name, name);

    g_rdpdr_fs.num_entries++;

    /* insert it in file system */
    g_rdpdr_fs.inode_table[xinode->inode] = xinode;
    xfuse_update_xrdpfs_size();
    log_debug("inserted new dir at inode_table[%d]", (int) xinode->inode);

    rdpr_fuse_dump_fs();

    log_debug("new inode=%d", (int) xinode->inode);

    /* setup return value */
    memset(&e, 0, sizeof(e));
    e.ino = xinode->inode;
    e.attr_timeout = RDPDR_FUSE_ATTR_TIMEOUT;
    e.entry_timeout = RDPDR_FUSE_ENTRY_TIMEOUT;
    e.attr.st_ino = xinode->inode;
    e.attr.st_mode = xinode->mode;
    e.attr.st_nlink = xinode->nlink;
    e.attr.st_uid = xinode->uid;
    e.attr.st_gid = xinode->gid;
    e.attr.st_size = 0;
    e.attr.st_atime = xinode->atime;
    e.attr.st_mtime = xinode->mtime;
    e.attr.st_ctime = xinode->ctime;
    e.generation = 1;

    fuse_reply_entry(req, &e);
}
#endif

static void rdpdr_fuse_dump_fs()
{
    int i;
    RDPDR_INODE *rinode;

    WLog_Print(g_logger, WLOG_DEBUG, "found %d entries", g_rdpdr_fs.num_entries - FIRST_INODE);

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        WLog_Print(g_logger, WLOG_DEBUG, "pinode=%d inode=%d nentries=%d nopen=%d is_synced=%d name=%s",
                  (int) rinode->parent_inode, (int) rinode->inode,
                  rinode->nentries, rinode->nopen, rinode->is_synced,
                  rinode->name);
    }
    WLog_Print(g_logger, WLOG_DEBUG, "");
}

/**
 * Dump contents of rdpdr inode structure
 *
 * @param rinode rdpdr inode structure to dump
 *****************************************************************************/

static void rdpdr_fuse_dump_inode(RDPDR_INODE *rinode)
{
    WLog_Print(g_logger, WLOG_DEBUG, "--- dumping struct rinode ---");
    WLog_Print(g_logger, WLOG_DEBUG, "name:          %s", rinode->name);
    WLog_Print(g_logger, WLOG_DEBUG, "parent_inode:  %ld", rinode->parent_inode);
    WLog_Print(g_logger, WLOG_DEBUG, "inode:         %ld", rinode->inode);
    WLog_Print(g_logger, WLOG_DEBUG, "mode:          %o", rinode->mode);
    WLog_Print(g_logger, WLOG_DEBUG, "nlink:         %d", rinode->nlink);
    WLog_Print(g_logger, WLOG_DEBUG, "uid:           %d", rinode->uid);
    WLog_Print(g_logger, WLOG_DEBUG, "gid:           %d", rinode->gid);
    WLog_Print(g_logger, WLOG_DEBUG, "size:          %ld", rinode->size);
    WLog_Print(g_logger, WLOG_DEBUG, "device_id:     %d", rinode->device_id);
    WLog_Print(g_logger, WLOG_DEBUG, "");
}

/**
 * Return the device_id associated with specified inode and copy the
 * full path to the specified inode into full_path
 *
 * @param ino the inode
 * @param full_path full path to the inode
 *
 * @return the device_id of specified inode
 *****************************************************************************/

static UINT32 rdpdr_fuse_get_device_id_for_inode(UINT32 ino, char *full_path)
{
    UINT32  parent_inode = 0;
    UINT32  child_inode  = ino;
    char    reverse_path[4096];

    /* ino == 1 is a special case; we already know that it is not */
    /* associated with any device redirection                     */
    if (ino == 1)
    {
        /* just return the device_id for the file in full_path */
        WLog_Print(g_logger, WLOG_DEBUG, "looking for file with pinode=%d name=%s", ino, full_path);
        rdpdr_fuse_dump_fs();

        RDPDR_INODE *rinode = rdpdr_fuse_get_rinode_from_pinode_name(ino, full_path);
        full_path[0] = 0;
        if (rinode)
            return rinode->device_id;
        else
            return 0;
    }

    reverse_path[0] = 0;
    full_path[0] = 0;

    while (1)
    {
        strcat(reverse_path, g_rdpdr_fs.inode_table[child_inode]->name);

        parent_inode = g_rdpdr_fs.inode_table[child_inode]->parent_inode;
        if (parent_inode == 1)
            break;

        strcat(reverse_path, "/");
        child_inode = parent_inode;
    }

    fuse_reverse_pathname(full_path, reverse_path);

    return g_rdpdr_fs.inode_table[child_inode]->device_id;
}

/**
 * Reverse the pathname in 'reverse_path' and insert it into 'full_path'
 *
 * Example: abba/music/share1 becomes share1/music/abba
 *
 * @param full_path path name in the correct order
 * @param reverse_path path name in the reverse order
 *****************************************************************************/

static void fuse_reverse_pathname(char *full_path, char *reverse_path)
{
    char *cptr;

    full_path[0] = 0;

    while ((cptr = strrchr(reverse_path, '/')) != NULL)
    {
        strcat(full_path, cptr + 1);
        strcat(full_path, "/");
        cptr[0] = 0;
    }
    strcat(full_path, reverse_path);
}

/**
 * Return the inode that matches the name and parent inode
 *****************************************************************************/

static RDPDR_INODE *rdpdr_fuse_get_rinode_from_pinode_name(UINT32 pinode,
                                                           const char *name)
{
    int i;
    RDPDR_INODE *rinode;

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* match parent inode */
        if (rinode->parent_inode != pinode)
            continue;

        /* match name */
        if (strcmp(rinode->name, name) != 0)
            continue;

        return rinode;
    }
    return NULL;
}

/**
 * Create file in file system
 *
 * @param pinode the parent inode
 * @param name   filename
 *
 * @return RDPDR_INODE on success, NULL on failure
 *****************************************************************************/

static RDPDR_INODE *rdpdr_fuse_create_file_in_fs(UINT32 device_id,
                                                 int pinode, char *name,
                                                 int type)
{
    RDPDR_INODE *rinode;
    RDPDR_INODE *rinodep;

    if ((name == NULL) || (strlen(name) == 0))
        return NULL;

    if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "system out of memory");
        return NULL;
    }

    rinode->parent_inode = pinode;
    rinode->inode = g_rdpdr_fs.next_node++;
    rinode->nlink = 1;
    rinode->uid = getuid();
    rinode->gid = getgid();
    rinode->atime = time(0);
    rinode->mtime = time(0);
    rinode->ctime = time(0);
    rinode->device_id = device_id;
    rinode->is_synced = 1;
    strcpy(rinode->name, name);

    if (type == S_IFDIR)
    {
        rinode->mode = 0755 | type;
        rinode->size = 4096;
    }
    else
    {
        rinode->mode = 0644 | type;
        rinode->size = 0;
    }

    g_rdpdr_fs.inode_table[rinode->inode] = rinode;
    g_rdpdr_fs.num_entries++;

    /* bump up lookup count in parent dir */
    rinodep = g_rdpdr_fs.inode_table[pinode];
    rinodep->nentries++;
    rdpdr_fuse_update_fs_size();

    WLog_Print(g_logger, WLOG_DEBUG, "incremented nentries; parent=%d nentries=%d",
              pinode, rinodep->nentries);

    return rinode;
}

/**
 * Check if specified file exists
 *
 * @param parent parent inode of file
 * @param name   flilename or dirname
 *
 * @return 1 if specified file exists, 0 otherwise
 *****************************************************************************/

static int rdpdr_fuse_does_file_exist(int parent, char *name)
{
    int          i;
    RDPDR_INODE *rinode;

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        if ((rinode->parent_inode == parent) &&
            (strcmp(rinode->name, name) == 0))
        {
            return 1;
        }
    }

    return 0;
}

static int rdpdr_fuse_delete_file(int parent, char *name)
{
    return -1;
}

static int rdpdr_fuse_delete_file_with_rinode(RDPDR_INODE *rinode)
{
    /* make sure it is not a dir */
    if ((rinode == NULL) || (rinode->mode & S_IFDIR))
        return -1;

    WLog_Print(g_logger, WLOG_DEBUG, "deleting: inode=%d name=%s", rinode->inode, rinode->name);

    g_rdpdr_fs.inode_table[rinode->parent_inode]->nentries--;
    g_rdpdr_fs.inode_table[rinode->inode] = NULL;
    free(rinode);

    return 0;
}

static int rdpdr_fuse_delete_dir_with_rinode(RDPDR_INODE *rinode)
{
    RDPDR_INODE *rip;
    int          i;

    /* make sure it is not a file */
    if ((rinode == NULL) || (rinode->mode & S_IFREG))
        return -1;

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rip = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* look for child inodes */
        if (rip->parent_inode == rinode->inode)
        {
            /* got one, delete it */
            g_rdpdr_fs.inode_table[rip->inode] = NULL;
            free(rip);
        }
    }

    /* our parent will have one less dir */
    g_rdpdr_fs.inode_table[rinode->parent_inode]->nentries--;

    g_rdpdr_fs.inode_table[rinode->inode] = NULL;
    free(rinode);

    return 0;
}

/**
 * Recursively delete dir with specified inode
 *****************************************************************************/

static int rdpdr_fuse_recursive_delete_dir_with_rinode(RDPDR_INODE *rinode)
{
    RDPDR_INODE *rip;
    int          i;

    /* make sure it is not a file */
    if ((rinode == NULL) || (rinode->mode & S_IFREG))
        return -1;

    WLog_Print(g_logger, WLOG_DEBUG, "recursively deleting dir with inode=%d name=%s",
              rinode->inode, rinode->name);

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rip = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* look for child inodes */
        if (rip->parent_inode == rinode->inode)
        {
            /* got one */
            if (rip->mode & S_IFREG)
            {
                /* regular file */
                g_rdpdr_fs.inode_table[rip->parent_inode]->nentries--;
                g_rdpdr_fs.inode_table[rip->inode] = NULL;
                free(rip);
            }
            else
            {
                /* got another dir */
                rdpdr_fuse_recursive_delete_dir_with_rinode(rip);
            }
        }
    }

    /* our parent will have one less dir */
    g_rdpdr_fs.inode_table[rinode->parent_inode]->nentries--;

    g_rdpdr_fs.inode_table[rinode->inode] = NULL;
    free(rinode);

    return 0;
}

static void rdpdr_fuse_update_fs_size()
{
    void *vp;
    int diff;

    diff = g_rdpdr_fs.max_entries - g_rdpdr_fs.num_entries;
    if (diff > 100)
        return;

    /* extend memory */
    vp = realloc(g_rdpdr_fs.inode_table,
                 (g_rdpdr_fs.max_entries + 100) * sizeof(RDPDR_INODE *));

    if (vp == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        return;
    }

    /* zero newly added memory */
    memset(vp + g_rdpdr_fs.max_entries * sizeof(RDPDR_INODE *),
           0,
           100 * sizeof(RDPDR_INODE *));

    g_rdpdr_fs.max_entries += 100;
    g_rdpdr_fs.inode_table = vp;
}

/******************************************************************************
**                                                                           **
**                         callbacks for devredir                            **
**                                                                           **
******************************************************************************/

/***************************************
 * Drive Create Directory Complete
 **************************************/

void rdpdr_fuse_on_drive_create_directory_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO *fip;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
        return;

    if (IoStatus != 0)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    fuse_reply_err(fip->req, 0);

    free(fip);
}

/***************************************
 * Drive Delete Directory Complete
 **************************************/

void rdpdr_fuse_on_drive_delete_directory_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO *fip;
    RDPDR_INODE     *rinode;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
        return;

    if (IoStatus != 0)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    /* now delete the item in file system */
    rinode = rdpdr_fuse_get_rinode_from_pinode_name(fip->inode, fip->name);
    if (rinode == NULL)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    g_rdpdr_fs.inode_table[rinode->inode] = NULL;
    free(rinode);

    /* update parent */
    rinode = g_rdpdr_fs.inode_table[fip->inode];
    rinode->nentries--;

    fuse_reply_err(fip->req, 0);
    free(fip);
}

/***************************************
 * Drive Query Directory Complete
 **************************************/

static void rdpdr_fuse_query_directory_add(void *callbackData, FILE_DIRECTORY_INFORMATION *fdi)
{
    RDPDR_FUSE_INFO *fip = (RDPDR_FUSE_INFO *) callbackData;
	RDPDR_INODE *rinode;

    if ((fip == NULL) || (fdi == NULL))
    {
        WLog_Print(g_logger, WLOG_ERROR, "fip or fdi are NULL");
        return;
    }

    if (!rdpdr_fuse_is_inode_valid(fip->inode))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", fip->inode);
        return;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "parent_inode=%d name=%s", fip->inode, fdi->FileName);

    /* if filename is . or .. don't add it */
    if ((strcmp(fdi->FileName, ".") == 0) || (strcmp(fdi->FileName, "..") == 0))
    {
        return;
    }

    rdpdr_fuse_dump_fs();

    if ((rinode = rdpdr_fuse_get_rinode_from_pinode_name(fip->inode, fdi->FileName)) != NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "inode=%d name=%s already exists in file system; not adding it",
                  fip->inode, rinode->name);
        rinode->stale = 0;
        return;
    }

	if ((rinode = calloc(1, sizeof(RDPDR_INODE))) == NULL)
	{
		WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
		return;
	}

	strcpy(rinode->name, fdi->FileName);
	rinode->size = (size_t) fdi->EndOfFile;
	rinode->mode = WINDOWS_TO_LINUX_FILE_PERM(fdi->FileAttributes);
	rinode->atime = WINDOWS_TO_LINUX_TIME(fdi->LastAccessTime);
	rinode->mtime = WINDOWS_TO_LINUX_TIME(fdi->LastWriteTime);
	rinode->ctime = WINDOWS_TO_LINUX_TIME(fdi->CreationTime);
	rinode->parent_inode = fip->inode;
	rinode->inode = g_rdpdr_fs.next_node++;
	rinode->uid = getuid();
	rinode->gid = getgid();
	rinode->device_id = fip->device_id;

    g_rdpdr_fs.num_entries++;

    /* insert it in file system and update lookup count */
    g_rdpdr_fs.inode_table[rinode->inode] = rinode;
    g_rdpdr_fs.inode_table[fip->inode]->nentries++;
    rdpdr_fuse_update_fs_size();

    rdpdr_fuse_dump_fs();
}

static void rdpdr_fuse_query_directory_done(void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO    *fip;
    struct dir_info    *di;
    struct opendir_req *odreq;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "fip is NULL");
        goto done;
    }

    if (IoStatus != 0)
    {
        /* command failed */
        if (fip->invoke_fuse)
            fuse_reply_err(fip->req, ENOENT);
        goto done;
    }

    /* do we have a valid inode? */
    if (!rdpdr_fuse_is_inode_valid(fip->inode))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", fip->inode);
        if (fip->invoke_fuse)
            fuse_reply_err(fip->req, EBADF);
        goto done;
    }

    rdpdr_fuse_delete_stale_entries(fip->inode);

    /* this will be used by xfuse_cb_readdir() */
    di = calloc(1, sizeof(struct dir_info));
    di->index = FIRST_INODE;
    fip->fi->fh = (long) di;

    fuse_reply_open(fip->req, fip->fi);

done:

    if (fip)
        free(fip);

    /* remove current request */
    free(Queue_Dequeue(g_opendir_queue));

    while (1)
    {
        /* process next request */
        odreq = Queue_Peek(g_opendir_queue);
        if (!odreq) break;

        if (rdpdr_fuse_proc_opendir_req(odreq->req, odreq->ino, odreq->fi))
            continue; /* request failed - process the next one */
        else
            break; /* request was queued */
    }
}

void rdpdr_fuse_on_drive_query_directory_complete(RdpdrServerContext *context, void *callbackData, UINT32 ioStatus, FILE_DIRECTORY_INFORMATION *fdi)
{
	if (ioStatus == STATUS_SUCCESS)
	{
		/* Skip hidden files. */
		if (fdi->FileAttributes & FILE_ATTRIBUTE_HIDDEN)
			return;

		rdpdr_fuse_query_directory_add(callbackData, fdi);
	}
	else
	{
		rdpdr_fuse_query_directory_done(callbackData, ioStatus == STATUS_NO_MORE_FILES ? STATUS_SUCCESS : ioStatus);
	}
}

/***************************************
 * Drive Open File Complete
 **************************************/

void rdpdr_fuse_on_drive_open_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus, UINT32 DeviceId, UINT32 FileId)
{
    RDPDR_FUSE_HANDLE *fh;
    RDPDR_INODE       *rinode;

    RDPDR_FUSE_INFO *fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "fip is NULL");
        goto done;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "+++ RDPDR_FUSE_INFO=%p RDPDR_FUSE_INFO->fi=%p DeviceId=%d FileId=%d",
              fip, fip->fi, DeviceId, FileId);

    if (IoStatus != 0)
    {
        if (!fip->invoke_fuse)
            goto done;

        switch (IoStatus)
        {
        case 0xC0000022:
            fuse_reply_err(fip->req, EACCES);
            break;

        case 0xC0000033:
        case 0xC0000034:
            fuse_reply_err(fip->req, ENOENT);
            break;

        default:
            fuse_reply_err(fip->req, EIO);
            break;
        }

        goto done;
    }

    if (fip->fi != NULL)
    {
        if ((fh = calloc(1, sizeof(RDPDR_FUSE_HANDLE))) == NULL)
        {
            WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
            if (fip->invoke_fuse)
                fuse_reply_err(fip->req, ENOMEM);

            free(fip);
            return;
        }

        /* save file handle for later use */
        fh->DeviceId = DeviceId;
        fh->FileId = FileId;

        fip->fi->fh = (uint64_t) ((long) fh);
        WLog_Print(g_logger, WLOG_DEBUG, "+++ RDPDR_FUSE_INFO=%p RDPDR_FUSE_INFO->fi=%p RDPDR_FUSE_INFO->fi->fh=%p",
                  fip, fip->fi, fip->fi->fh);
    }

    if (fip->invoke_fuse)
    {
        if (fip->reply_type == RT_FUSE_REPLY_OPEN)
        {
            WLog_Print(g_logger, WLOG_DEBUG, "sending fuse_reply_open(); "
                      "DeviceId=%d FileId=%d req=%p fi=%p",
                      fh->DeviceId, fh->FileId, fip->req, fip->fi);

            /* update open count */
            if ((rinode = g_rdpdr_fs.inode_table[fip->inode]) != NULL)
                rinode->nopen++;

            fuse_reply_open(fip->req, fip->fi);
        }
        else if (fip->reply_type == RT_FUSE_REPLY_CREATE)
        {
            struct fuse_entry_param  e;

// LK_TODO
#if 0
            if ((rinode = g_rdpdr_fs.inode_table[fip->inode]) == NULL)
            {
                WLog_Print(g_logger, WLOG_ERROR, "inode at inode_table[%d] is NULL", fip->inode);
                fuse_reply_err(fip->req, EBADF);
                goto done;
            }
#else
            /* create entry in file system */
            rinode = rdpdr_fuse_create_file_in_fs(fip->device_id, fip->inode,
                                                  fip->name, fip->mode);
            if (rinode == NULL)
            {
                fuse_reply_err(fip->req, ENOMEM);
                return;
            }
#endif
            memset(&e, 0, sizeof(struct fuse_entry_param));

            e.ino = rinode->inode;
            e.attr_timeout = RDPDR_FUSE_ATTR_TIMEOUT;
            e.entry_timeout = RDPDR_FUSE_ENTRY_TIMEOUT;
            e.attr.st_ino = rinode->inode;
            e.attr.st_mode = rinode->mode;
            e.attr.st_nlink = rinode->nlink;
            e.attr.st_uid = rinode->uid;
            e.attr.st_gid = rinode->gid;
            e.attr.st_size = rinode->size;
            e.attr.st_atime = rinode->atime;
            e.attr.st_mtime = rinode->mtime;
            e.attr.st_ctime = rinode->ctime;
            e.generation = 1;

            if (fip->mode == S_IFDIR)
            {
                fuse_reply_entry(fip->req, &e);
            }
            else
            {
                rinode->nopen++;
                fuse_reply_create(fip->req, &e, fip->fi);
            }
        }
        else
        {
            WLog_Print(g_logger, WLOG_ERROR, "invalid reply type: %d", fip->reply_type);
        }
    }

done:

    free(fip);
}

/***************************************
 * Drive Read File Complete
 **************************************/

void rdpdr_fuse_on_drive_read_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus, const char *buf, size_t length)
{
    RDPDR_FUSE_INFO   *fip;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if ((fip == NULL) || (fip->req == NULL))
    {
        WLog_Print(g_logger, WLOG_ERROR, "fip for fip->req is NULL");
        return;
    }

    fuse_reply_buf(fip->req, buf, length);
    free(fip);
}

/***************************************
 * Drive Write File Complete
 **************************************/

void rdpdr_fuse_on_drive_write_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus, size_t length)
{
    RDPDR_INODE     *rinode;
    RDPDR_FUSE_INFO *fip;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if ((fip == NULL) || (fip->req == NULL) || (fip->fi == NULL))
    {
        WLog_Print(g_logger, WLOG_ERROR, "fip, fip->req or fip->fi is NULL");
        return;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "+++ RDPDR_FUSE_INFO=%p, RDPDR_FUSE_INFO->fi=%p RDPDR_FUSE_INFO->fi->fh=%p",
              fip, fip->fi, fip->fi->fh);

    fuse_reply_write(fip->req, length);

    /* update file size */
    if ((rinode = g_rdpdr_fs.inode_table[fip->inode]) != NULL)
        rinode->size += length;
    else
        WLog_Print(g_logger, WLOG_ERROR, "inode at inode_table[%d] is NULL", fip->inode);

    free(fip);
}

/***************************************
 * Drive Close File Complete
 **************************************/

void rdpdr_fuse_on_drive_close_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO *fip;
    RDPDR_INODE     *rinode;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "fip is NULL");
        return;
    }

    if (fip->fi == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "fip->fi is NULL");
        return;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "+++ RDPDR_FUSE_INFO=%p RDPDR_FUSE_INFO->fi=%p RDPDR_FUSE_INFO->fi->fh=%p",
              fip, fip->fi, fip->fi->fh);

    if ((rinode = g_rdpdr_fs.inode_table[fip->inode]) == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "inode_table[%d] is NULL", fip->inode);
        fuse_reply_err(fip->req, EBADF);
        return;
    }

    WLog_Print(g_logger, WLOG_DEBUG, "before: inode=%d nopen=%d", rinode->inode, rinode->nopen);

    if (rinode->nopen > 0)
        rinode->nopen--;

    /* LK_TODO */
#if 0
    if ((rinode->nopen == 0) && fip->fi && fip->fi->fh)
    {
        printf("LK_TODO: ################################ fi=%p fi->fh=%p\n",
               fip->fi, fip->fi->fh);

        free((char *) (fip->fi->fh));
        fip->fi->fh = NULL;
    }
#endif

    fuse_reply_err(fip->req, 0);
}

/***************************************
 * Drive Delete File Complete
 **************************************/

void rdpdr_fuse_on_drive_delete_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO *fip;
    RDPDR_INODE     *rinode;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
        return;

    if (IoStatus != 0)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    /* now delete the item in file system */
    rinode = rdpdr_fuse_get_rinode_from_pinode_name(fip->inode, fip->name);
    if (rinode == NULL)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    g_rdpdr_fs.inode_table[rinode->inode] = NULL;
    free(rinode);

    /* update parent */
    rinode = g_rdpdr_fs.inode_table[fip->inode];
    rinode->nentries--;

    fuse_reply_err(fip->req, 0);
    free(fip);
}

/***************************************
 * Drive Rename File Complete
 **************************************/

void rdpdr_fuse_on_drive_rename_file_complete(RdpdrServerContext *context, void *callbackData, UINT32 IoStatus)
{
    RDPDR_FUSE_INFO *fip;
    RDPDR_INODE     *old_rinode;
    RDPDR_INODE     *new_rinode;

    fip = (RDPDR_FUSE_INFO *) callbackData;
    if (fip == NULL)
        return;

    if (IoStatus != 0)
    {
        fuse_reply_err(fip->req, EEXIST);
        free(fip);
        return;
    }

    /*
     * update file system
     */

    /* if destination dir/file exists, delete it */
    if (rdpdr_fuse_does_file_exist(fip->new_inode, fip->new_name))
    {
        new_rinode = rdpdr_fuse_get_rinode_from_pinode_name(fip->new_inode,
                                                      fip->new_name);

        if (new_rinode->mode & S_IFREG)
            rdpdr_fuse_delete_file_with_rinode(new_rinode);
        else
            rdpdr_fuse_delete_dir_with_rinode(new_rinode);

        new_rinode = NULL;
    }

    old_rinode = rdpdr_fuse_get_rinode_from_pinode_name(fip->inode, fip->name);
    if (old_rinode == NULL)
    {
        fuse_reply_err(fip->req, EBADF);
        free(fip);
        return;
    }

    old_rinode->parent_inode = fip->new_inode;
    strcpy(old_rinode->name, fip->new_name);

    if (fip->inode != fip->new_inode)
    {
        /* file has been moved to a different dir */
        old_rinode->is_synced = 1;
        g_rdpdr_fs.inode_table[fip->inode]->nentries--;
        g_rdpdr_fs.inode_table[fip->new_inode]->nentries++;
    }

    fuse_reply_err(fip->req, 0);
    free(fip);
}

/******************************************************************************
**                                                                           **
**                           callbacks for fuse                              **
**                                                                           **
******************************************************************************/

/**
 * Look up a directory entry by name and get its attributes
 *
 *****************************************************************************/

static void rdpdr_fuse_cb_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    RDPDR_INODE             *rinode;
    struct fuse_entry_param  e;

    WLog_Print(g_logger, WLOG_DEBUG, "looking for parent=%d name=%s", (int) parent, name);

    //rdpdr_fuse_dump_fs();

    if (!rdpdr_fuse_is_inode_valid(parent))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", parent);
        fuse_reply_err(req, EBADF);
        return;
    }

    rinode = rdpdr_fuse_get_rinode_from_pinode_name(parent, name);
    if (rinode == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "did not find entry for parent=%d name=%s", parent, name);
        fuse_reply_err(req, ENOENT);
        return;
    }

    memset(&e, 0, sizeof(e));
    e.ino = rinode->inode;
    e.attr_timeout = RDPDR_FUSE_ATTR_TIMEOUT;
    e.entry_timeout = RDPDR_FUSE_ENTRY_TIMEOUT;
    e.attr.st_ino = rinode->inode;
    e.attr.st_mode = rinode->mode;
    e.attr.st_nlink = rinode->nlink;
    e.attr.st_uid = rinode->uid;
    e.attr.st_gid = rinode->gid;
    e.attr.st_size = rinode->size;
    e.attr.st_atime = rinode->atime;
    e.attr.st_mtime = rinode->mtime;
    e.attr.st_ctime = rinode->ctime;
    e.generation = 1;

    fuse_reply_entry(req, &e);

    WLog_Print(g_logger, WLOG_DEBUG, "found entry for parent=%d name=%s uid=%d gid=%d",
              parent, name, rinode->uid, rinode->gid);

    return;
}

/**
 * Get file attributes
 *****************************************************************************/

static void rdpdr_fuse_cb_getattr(fuse_req_t req, fuse_ino_t ino,
                            struct fuse_file_info *fi)
{
    RDPDR_INODE *rinode;
    struct stat  stbuf;

    (void) fi;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: req=%p ino=%d\n", __FUNCTION__, req, (int) ino); fflush(stdout);

    /* if ino is not valid, just return */
    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    rinode = g_rdpdr_fs.inode_table[ino];
    if (!rinode)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "****** invalid ino=%d", (int) ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    stbuf.st_mode = rinode->mode;
    stbuf.st_nlink = rinode->nlink;
    stbuf.st_size = rinode->size;

    fuse_reply_attr(req, &stbuf, 1.0);
}

/**
 *
 *****************************************************************************/
#define USE_DIRBUF 0

#if USE_DIRBUF
static void rdpdr_fuse_dirbuf_add(fuse_req_t req, struct dirbuf *b,
                             const char *name, fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = b->size;

    WLog_Print(g_logger, WLOG_DEBUG, "adding ino=%d name=%s", (int) ino, name);

    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    b->p = (char *) realloc(b->p, b->size);

    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                      b->size);
}

static int rdpdr_fuse_reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize, off_t off, size_t maxsize)
{
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

#else

static int rdpdr_fuse_dirbuf1_add(fuse_req_t req, struct dirbuf1 *b,
                             const char *name, fuse_ino_t ino)
{
    struct stat  stbuf;
    int          len;

    len = fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    if (b->bytes_in_buf + len > 4096)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "not adding entry because dirbuf overflow would occur");
        return -1;
    }

    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;

    fuse_add_direntry(req,
                      &b->buf[b->bytes_in_buf], /* index where new entry will be added to buf */
                      4096 - len,               /* remaining size of buf */
                      name,                     /* name of entry */
                      &stbuf,                   /* file attributes */
                      b->bytes_in_buf + len     /* offset of next entry */
                     );

    b->bytes_in_buf += len;
    return 0;
}

#endif

/**
 *
 *****************************************************************************/

static void rdpdr_fuse_cb_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                             off_t off, struct fuse_file_info *fi)
{
    RDPDR_INODE     *rinode;
    RDPDR_INODE     *ti;
    struct dir_info *di;
#if USE_DIRBUF
    struct dirbuf    b;
#else
    struct dirbuf1   b;
#endif
    int              i;
    int              first_time;

    WLog_Print(g_logger, WLOG_DEBUG, "req=%p inode=%d size=%d offset=%d, fi=%p", req, ino, size, off, fi); fflush(stdout);

    /* do we have a valid inode? */
    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

	memset(&b, 0, sizeof(b));

#if USE_DIRBUF
	first_time = 1;

	for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
#else
    di = (struct dir_info *) (LONG_PTR) fi->fh;
    if (di == NULL)
    {
        /* something seriously wrong somewhere! */
        fuse_reply_buf(req, 0, 0);
        return;
    }

    b.bytes_in_buf = 0;
    first_time = (di->index == FIRST_INODE) ? 1 : 0;

    for (i = di->index; i < g_rdpdr_fs.num_entries; i++, di->index++)
#endif
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* match parent inode */
        if (rinode->parent_inode != ino)
            continue;

        rinode->is_synced = 1;

        if (first_time)
        {
            first_time = 0;
            ti = g_rdpdr_fs.inode_table[ino];
            if (!ti)
            {
                WLog_Print(g_logger, WLOG_DEBUG, "****** g_rdpds_fs.inode_table[%d] is NULL", ino);
                fuse_reply_buf(req, NULL, 0);
                return;
            }
#if USE_DIRBUF
            rdpdr_fuse_dirbuf_add(req, &b, ".", ino);
            rdpdr_fuse_dirbuf_add(req, &b, "..", ti->parent_inode);
#else
            rdpdr_fuse_dirbuf1_add(req, &b, ".", ino);
            rdpdr_fuse_dirbuf1_add(req, &b, "..", ti->parent_inode);
#endif
        }

#if USE_DIRBUF
        rdpdr_fuse_dirbuf_add(req, &b, rinode->name, rinode->inode);
#else
        if (rdpdr_fuse_dirbuf1_add(req, &b, rinode->name, rinode->inode))
            break; /* buffer is full */
#endif
    }

#if USE_DIRBUF
    rdpdr_fuse_reply_buf_limited(req, b.p, b.size, off, size);
#else
    if (b.bytes_in_buf)
        fuse_reply_buf(req, b.buf, b.bytes_in_buf);
    else
        fuse_reply_buf(req, NULL, 0);
#endif
}

/**
 * Create a directory
 *****************************************************************************/

static void rdpdr_fuse_cb_mkdir(fuse_req_t req, fuse_ino_t parent,
                           const char *name, mode_t mode)
{
    RDPDR_INODE              *rinode;
    struct fuse_entry_param   e;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: parent=%d name=%s mode=%d",
        __FUNCTION__, (int) parent, name, (int) mode);

    if ((rinode = rdpdr_fuse_get_rinode_from_pinode_name(parent, name)) != NULL)
    {
        /* dir already exists, just return it */
        memset(&e, 0, sizeof(struct fuse_entry_param));

        e.ino = rinode->inode;
        e.attr_timeout = RDPDR_FUSE_ATTR_TIMEOUT;
        e.entry_timeout = RDPDR_FUSE_ENTRY_TIMEOUT;
        e.attr.st_ino = rinode->inode;
        e.attr.st_mode = rinode->mode;
        e.attr.st_nlink = rinode->nlink;
        e.attr.st_uid = rinode->uid;
        e.attr.st_gid = rinode->gid;
        e.attr.st_size = rinode->size;
        e.attr.st_atime = rinode->atime;
        e.attr.st_mtime = rinode->mtime;
        e.attr.st_ctime = rinode->ctime;
        e.generation = 1;

        fuse_reply_entry(req, &e);

        return;
    }

    /* dir does not exist, create it */
    rdpdr_fuse_create_dir_or_file(req, parent, name, mode, NULL, S_IFDIR);
}

/**
 * Remove specified dir
 *****************************************************************************/

static void rdpdr_fuse_cb_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    rdpdr_fuse_remove_dir_or_file(req, parent, name, 1);
}

static void rdpdr_fuse_cb_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    rdpdr_fuse_remove_dir_or_file(req, parent, name, 2);
}

/**
 * Remove a dir or file
 *
 * @param type 1=dir, 2=file
 *****************************************************************************/

static void rdpdr_fuse_remove_dir_or_file(fuse_req_t req, fuse_ino_t parent, const char *name, int type)
{
    RDPDR_FUSE_INFO *fip;
    RDPDR_INODE     *rinode;
    char            *cptr;
    char             full_path[4096];
    UINT32           device_id;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: parent=%d name=%s, type=%d", __FUNCTION__, parent, name, type);

    /* is parent inode valid? */
    if (!rdpdr_fuse_is_inode_valid(parent))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", parent);
        fuse_reply_err(req, EBADF);
        return;
    }

    if ((rinode = rdpdr_fuse_get_rinode_from_pinode_name(parent, name)) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "did not find file with pinode=%d name=%s", parent, name);
        fuse_reply_err(req, EBADF);
        return;
    }

    device_id = rdpdr_fuse_get_device_id_for_inode(parent, full_path);

    WLog_Print(g_logger, WLOG_DEBUG, "path=%s nentries=%d", full_path, rinode->nentries);

    if ((type == 1) && (rinode->nentries != 0))
    {
        WLog_Print(g_logger, WLOG_DEBUG, "cannot rmdir; lookup count is %d", rinode->nentries);
        fuse_reply_err(req, ENOTEMPTY);
        return;
    }
    else if (type == 2)
    {
        if ((rinode->nopen > 1) || ((rinode->nopen == 1) &&
                                    (rinode->close_in_progress == 0)))
        {
            WLog_Print(g_logger, WLOG_DEBUG, "cannot unlink; open count is %d", rinode->nopen);
            fuse_reply_err(req, EBUSY);
            return;
        }
    }

    strcat(full_path, "/");
    strcat(full_path, name);

    if (rinode->is_loc_resource)
    {
        /* specified file is a local resource */
        //XFUSE_HANDLE *fh;

        WLog_Print(g_logger, WLOG_DEBUG, "LK_TODO: this is still a TODO");
        fuse_reply_err(req, EINVAL);
        return;
    }

    /* specified file resides on redirected share */

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fip->req = req;
    fip->inode = parent;
    fip->invoke_fuse = 1;
    fip->device_id = device_id;
    strncpy(fip->name, name, 1024);
    fip->name[1023] = 0;
    fip->type = type;

	/* Skip over the root node of the share. */
	if ((cptr = strchr(full_path, '/')) == NULL)
	{
		/* Use the root directory. */
		cptr = "/";
	}

    switch (type)
    {
        case 1: /* directory */
            /* Send a request to delete the directory. */
            if (!g_rdpdrServerContext->DriveDeleteDirectory(g_rdpdrServerContext, (void *) fip, device_id, cptr))
            {
                WLog_Print(g_logger, WLOG_ERROR, "failed to send delete directory command");
                fuse_reply_err(req, EREMOTEIO);
                free(fip);
            }
            break;

        case 2: /* file */
            /* Send a request to delete the file. */
            if (!g_rdpdrServerContext->DriveDeleteFile(g_rdpdrServerContext, (void *) fip, device_id, cptr))
            {
                WLog_Print(g_logger, WLOG_ERROR, "failed to send delete file command");
                fuse_reply_err(req, EREMOTEIO);
                free(fip);
            }
            break;
    }
}

static void rdpdr_fuse_cb_rename(fuse_req_t req,
                            fuse_ino_t old_parent, const char *old_name,
                            fuse_ino_t new_parent, const char *new_name)
{
    RDPDR_INODE     *old_rinode;
    RDPDR_FUSE_INFO *fip;
    UINT32           new_device_id;
    char            *cptr;
    char             old_full_path[1024];
    char             new_full_path[1024];
    char            *cp;

    UINT32           device_id;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: old_parent=%d old_name=%s new_parent=%d new_name=%s",
        __FUNCTION__, (int) old_parent, old_name, (int) new_parent, new_name);

    rdpdr_fuse_dump_fs();

    /* is old_parent inode valid? */
    if (!rdpdr_fuse_is_inode_valid(old_parent))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", old_parent);
        fuse_reply_err(req, EINVAL);
        return;
    }

    /* is new_parent inode valid? */
    if (!rdpdr_fuse_is_inode_valid(new_parent))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", new_parent);
        fuse_reply_err(req, EINVAL);
        return;
    }

    if ((old_name == NULL) || (strlen(old_name) == 0))
    {
        fuse_reply_err(req, EINVAL);
        return;
    }

    if ((new_name == NULL) || (strlen(new_name) == 0))
    {
        fuse_reply_err(req, EINVAL);
        return;
    }

    old_rinode = rdpdr_fuse_get_rinode_from_pinode_name(old_parent, old_name);
    if (old_rinode  == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "did not find file with pinode=%d name=%s",
                  old_parent, old_name);
        fuse_reply_err(req, EBADF);
        return;
    }

    /* if file is open, cannot rename */
    if (old_rinode->nopen != 0)
    {
        fuse_reply_err(req, EBUSY);
        return;
    }

    /* rename across file systems not yet supported */
    new_device_id = rdpdr_fuse_get_device_id_for_inode(new_parent, new_full_path);
    strcat(new_full_path, "/");
    strcat(new_full_path, new_name);

    if (new_device_id != old_rinode->device_id)
    {
        WLog_Print(g_logger, WLOG_ERROR, "rename across file systems not supported");
        fuse_reply_err(req, EINVAL);
        return;
    }

    if (old_rinode->is_loc_resource)
    {
        /* specified file is a local resource */
        WLog_Print(g_logger, WLOG_DEBUG, "LK_TODO: this is still a TODO");
        fuse_reply_err(req, EINVAL);
        return;
    }

    /* resource is on a redirected share */

    device_id = old_rinode->device_id;

    rdpdr_fuse_get_device_id_for_inode(old_parent, old_full_path);
    strcat(old_full_path, "/");
    strcat(old_full_path, old_name);

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fip->req = req;
    fip->inode = old_parent;
    fip->new_inode = new_parent;
    strncpy(fip->name, old_name, 1024);
    strncpy(fip->new_name, new_name, 1024);
    fip->name[1023] = 0;
    fip->new_name[1023] = 0;
    fip->invoke_fuse = 1;
    fip->device_id = device_id;

    if ((cp = strchr(new_full_path, '/')) == NULL)
        cp = "\\";

    /* we want path minus 'root node of the share' */
    if ((cptr = strchr(old_full_path, '/')) == NULL)
    {
        /* get dev_redir to open the remote file */
        if (!g_rdpdrServerContext->DriveRenameFile(g_rdpdrServerContext, (void *) fip, device_id, "\\", cp))
        {
            WLog_Print(g_logger, WLOG_ERROR, "failed to send rename file command");
            fuse_reply_err(req, EREMOTEIO);
            free(fip);
            return;
        }
    }
    else
    {
        if (!g_rdpdrServerContext->DriveRenameFile(g_rdpdrServerContext, (void *) fip, device_id, cptr, cp))
        {
            WLog_Print(g_logger, WLOG_ERROR, "failed to send rename file command");
            fuse_reply_err(req, EREMOTEIO);
            free(fip);
            return;
        }
    }
}

/**
 * Create a directory or file
 *
 * @param req    opaque FUSE object
 * @param parent parent inode
 * @param name   name of dir or file to create
 * @param mode   creation mode
 * @param fi     for storing file handles
 * @param type   S_IFDIR for dir and S_IFREG for file
 *****************************************************************************/

static void rdpdr_fuse_create_dir_or_file(fuse_req_t req, fuse_ino_t parent,
                                     const char *name, mode_t mode,
                                     struct fuse_file_info *fi, int type)
{
    RDPDR_FUSE_INFO   *fip;
    char              *cptr;
    char               full_path[1024];
    UINT32             device_id;

    full_path[0] = 0;

    WLog_Print(g_logger, WLOG_DEBUG, "entered: parent_ino=%d name=%s mode=%d type=%s",
              (int) parent, name, (int) mode, (type == S_IFDIR) ? "dir" : "file");

    /* name must be valid */
    if ((name == NULL) || (strlen(name) == 0))
    {
        WLog_Print(g_logger, WLOG_ERROR, "invalid name");
        fuse_reply_err(req, EBADF);
        return;
    }

    /* is parent inode valid? */
    if ((parent == 1) || (!rdpdr_fuse_is_inode_valid(parent)))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", parent);
        fuse_reply_err(req, EBADF);
        return;
    }

    device_id = rdpdr_fuse_get_device_id_for_inode(parent, full_path);
    strcat(full_path, "/");
    strcat(full_path, name);

    RDPDR_INODE *rinode = g_rdpdr_fs.inode_table[parent];
    if (rinode->is_loc_resource)
    {
        /* specified file is a local resource */
        //XFUSE_HANDLE *fh;

        WLog_Print(g_logger, WLOG_DEBUG, "LK_TODO: this is still a TODO");
        fuse_reply_err(req, EINVAL);
        return;
    }

    /* specified file resides on redirected share */

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
       WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
       fuse_reply_err(req, ENOMEM);
       return;
    }

    fip->req = req;
    fip->fi = fi;
    fip->inode = parent;
    fip->invoke_fuse = 1;
    fip->device_id = device_id;
    fip->mode = type;
    fip->reply_type = RT_FUSE_REPLY_CREATE;
    strncpy(fip->name, name, 1024);
    fip->name[1023] = 0;

    WLog_Print(g_logger, WLOG_DEBUG, "+++ created RDPDR_FUSE_INFO=%p RDPDR_FUSE_INFO->fi=%p", fip, fip->fi);

    /* LK_TODO need to handle open permissions */

	/* Skip over the root node of the share. */
	if ((cptr = strchr(full_path, '/')) == NULL)
	{
		/* Use the root directory. */
		cptr = "/";
	}

    if (type == S_IFDIR)
    {
        /* Send a request to create the directory on the client. */
        if (!g_rdpdrServerContext->DriveCreateDirectory(g_rdpdrServerContext, (void *) fip, device_id, cptr))
        {
            WLog_Print(g_logger, WLOG_ERROR, "failed to send drive create directory cmd");
            fuse_reply_err(req, EREMOTEIO);
        }
    }
    else
    {
        /* Send a request to create the file on the client. */
        if (!g_rdpdrServerContext->DriveOpenFile(g_rdpdrServerContext, (void *) fip, device_id, cptr, FILE_WRITE_DATA, FILE_CREATE))
        {
            WLog_Print(g_logger, WLOG_ERROR, "failed to send drive open file cmd");
            fuse_reply_err(req, EREMOTEIO);
        }
    }
}

/**
 * Open specified file
 *****************************************************************************/

static void rdpdr_fuse_cb_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    RDPDR_INODE       *rinode;
    RDPDR_FUSE_INFO   *fip;
    char              *cptr;
    char               full_path[4096];
    UINT32             device_id;
    UINT32             desired_access;
    UINT32             create_disposition;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d", __FUNCTION__, (int) ino);

    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    /* if ino points to a dir, fail the open request */
    rinode = g_rdpdr_fs.inode_table[ino];
    if (!rinode)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "****** g_rdpdr_fs.inode_table[%d] is NULL", ino);
        fuse_reply_err(req, EBADF);
        return;
    }
    if (rinode->mode & S_IFDIR)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "reading a dir not allowed!");
        fuse_reply_err(req, EISDIR);
        return;
    }

    device_id = rdpdr_fuse_get_device_id_for_inode((UINT32) ino, full_path);

    if (rinode->is_loc_resource)
    {
        /* specified file is a local resource */
        RDPDR_FUSE_HANDLE *fh = calloc(1, sizeof(RDPDR_FUSE_HANDLE));
        fh->is_loc_resource = 1;
        fi->fh = (uint64_t) ((long) fh);
        fuse_reply_open(req, fi);
        return;
    }

    /* specified file resides on redirected share */

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
       WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
       fuse_reply_err(req, ENOMEM);
       return;
    }

    fip->req = req;
    fip->inode = ino;
    fip->invoke_fuse = 1;
    fip->device_id = device_id;
    fip->fi = fi;

    WLog_Print(g_logger, WLOG_DEBUG, "LK_TODO: fip->fi = %p", fip->fi);

    strncpy(fip->name, full_path, 1024);
    fip->name[1023] = 0;
    fip->reply_type = RT_FUSE_REPLY_OPEN;

	if (fi->flags & O_CREAT)
	{
		create_disposition = FILE_CREATE;
		if (fi->flags & O_TRUNC)
		{
			desired_access = FILE_WRITE_DATA;
		}
		else
		{
			desired_access = FILE_APPEND_DATA;
		}
	}
	else
	{
		create_disposition = FILE_OPEN;
		if (fi->flags & O_RDONLY)
		{
			desired_access = FILE_READ_DATA;
		}
		else
		{
			desired_access = FILE_WRITE_DATA;
		}
	}

	/* Skip over the root node of the share. */
	if ((cptr = strchr(full_path, '/')) == NULL)
	{
		/* Use the root directory. */
		cptr = "/";
	}

    /* Send a request to open the file on the client. */
    if (!g_rdpdrServerContext->DriveOpenFile(g_rdpdrServerContext, (void *) fip, device_id, cptr, desired_access, create_disposition))
    {
        WLog_Print(g_logger, WLOG_ERROR, "failed to send drive file open cmd");
        fuse_reply_err(req, EREMOTEIO);
    }
}

static void rdpdr_fuse_cb_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    RDPDR_FUSE_INFO   *fip    = NULL;
    RDPDR_FUSE_HANDLE *handle = (RDPDR_FUSE_HANDLE *) (LONG_PTR) fi->fh;
    UINT32             FileId;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d fi=%p fi->fh=%p",
        __FUNCTION__, (int) ino, fi, fi->fh);

    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    RDPDR_INODE *rinode = g_rdpdr_fs.inode_table[ino];
    if (!rinode)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "****** g_rdpdr_fs.inode_table[%d] is NULL", ino);
        fuse_reply_err(req, 0);
        return;
    }
    if (rinode->is_loc_resource)
    {
        /* specified file is a local resource */
        fuse_reply_err(req, 0);
        return;
    }

    /* specified file resides on redirected share */

    WLog_Print(g_logger, WLOG_DEBUG, "nopen=%d", rinode->nopen);

    /* if file is not opened, just return */
    if (rinode->nopen == 0)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "cannot close because file not opened");
        fuse_reply_err(req, 0);
        return;
    }

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fip->req = req;
    fip->inode = ino;
    fip->invoke_fuse = 1;
    fip->device_id = handle->DeviceId;
    fip->fi = fi;

    WLog_Print(g_logger, WLOG_DEBUG, " +++ created XFUSE_INFO=%p XFUSE_INFO->fi=%p XFUSE_INFO->fi->fh=%p",
              fip, fip->fi, fip->fi->fh);

    FileId = handle->FileId;
    free(handle);
    fip->fi->fh = 0;
    rinode->close_in_progress = 1;

    if (!g_rdpdrServerContext->DriveCloseFile(g_rdpdrServerContext, (void *) fip, fip->device_id, handle->FileId))
    {
        WLog_Print(g_logger, WLOG_ERROR, "failed to send devredir_close_file() cmd");
        fuse_reply_err(req, EREMOTEIO);
    }
}

/**
 *****************************************************************************/

static void rdpdr_fuse_cb_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, struct fuse_file_info *fi)
{
    RDPDR_FUSE_HANDLE     *fh;
    RDPDR_FUSE_INFO       *fusep;
    RDPDR_INODE           *rinode;
    struct req_list_item  *rli;
    long                   handle;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d size=%d off=%d fi=%p",
        __FUNCTION__, (int) ino, (int) size, (int) off, fi);

	if (fi == NULL)
	{
		WLog_Print(g_logger, WLOG_DEBUG, "fi is NULL - something seriously wrong"); fflush(stdout);
		fuse_reply_err(req, EINVAL);
		return;
	}

    if (fi->fh == 0)
    {
        fuse_reply_err(req, EINVAL);
        return;
    }

    handle = fi->fh;
    fh = (RDPDR_FUSE_HANDLE *) handle;

    if (fh->is_loc_resource)
    {
        /* target file is in .clipboard dir */

        WLog_Print(g_logger, WLOG_DEBUG, "target file is in .clipboard dir");

        if ((rinode = g_rdpdr_fs.inode_table[ino]) == NULL)
        {
            WLog_Print(g_logger, WLOG_ERROR, "ino does not exist in file system");
            fuse_reply_buf(req, 0, 0);
            return;
        }

        rli = (struct req_list_item *) malloc(sizeof(struct req_list_item));

        rli->stream_id = 0;
        rli->req = req;
        rli->lindex = rinode->lindex;
        rli->off = off;
        rli->size = size;
        ArrayList_Add(g_req_list, rli);

        if (ArrayList_Count(g_req_list) == 1)
        {
            WLog_Print(g_logger, WLOG_DEBUG, "requesting clipboard file data lindex = %d off = %d size = %d",
                      rli->lindex, (int) off, (int) size);

            clipboard_request_file_data(rli->stream_id, rli->lindex,
                                        (int) off, (int) size);
        }

        return;
    }

    /* target file is on a remote device */

    if ((fusep = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fusep->req = req;
    fusep->inode = ino;
    fusep->invoke_fuse = 1;
    fusep->device_id = fh->DeviceId;
    fusep->fi = fi;

    g_rdpdrServerContext->DriveReadFile(g_rdpdrServerContext, fusep, fh->DeviceId, fh->FileId, size, off);
}

/**
 *****************************************************************************/

static void rdpdr_fuse_cb_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                           size_t size, off_t off, struct fuse_file_info *fi)
{
    RDPDR_FUSE_HANDLE *fh;
    RDPDR_FUSE_INFO   *fusep;
    long               handle;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d size=%d off=%d",
        __FUNCTION__, (int) ino, (int) size, (int) off);

    if (fi->fh == 0)
    {
        WLog_Print(g_logger, WLOG_ERROR, "file handle fi->fh is NULL");
        fuse_reply_err(req, EINVAL);
        return;
    }

    handle = fi->fh;
    fh = (RDPDR_FUSE_HANDLE *) handle;

    if (fh->is_loc_resource)
    {
        /* target file is in .clipboard dir */
        WLog_Print(g_logger, WLOG_DEBUG, "THIS IS STILL A TODO!");
        return;
    }

    /* target file is on a remote device */

    if ((fusep = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fusep->req = req;
    fusep->inode = ino;
    fusep->invoke_fuse = 1;
    fusep->device_id = fh->DeviceId;
    fusep->fi = fi;

    WLog_Print(g_logger, WLOG_DEBUG, "+++ created RDPDR_FUSE_INFO=%p RDPDR_FUSE_INFO->fi=%p RDPDR_FUSE_INFO->fi->fh=%p",
              fusep, fusep->fi, fusep->fi->fh);

    g_rdpdrServerContext->DriveWriteFile(g_rdpdrServerContext, fusep, fh->DeviceId, fh->FileId, buf, size, off);

    WLog_Print(g_logger, WLOG_DEBUG, "exiting");
}

/**
 *****************************************************************************/

static void rdpdr_fuse_cb_create(fuse_req_t req, fuse_ino_t parent,
                            const char *name, mode_t mode,
                            struct fuse_file_info *fi)
{
    WLog_Print(g_logger, WLOG_DEBUG, "%s: parent=%d, name=%s mode=%d fi=%p",
        __FUNCTION__, (int) parent, name, (int) mode, fi);

    rdpdr_fuse_create_dir_or_file(req, parent, name, mode, fi, S_IFREG);
}

/**
 *****************************************************************************/

static void rdpdr_fuse_cb_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                           struct fuse_file_info *fi)
{
    WLog_Print(g_logger, WLOG_DEBUG, "#################### entered: ino=%d datasync=%d", (int) ino, datasync);
    WLog_Print(g_logger, WLOG_DEBUG, "function not required");
    fuse_reply_err(req, EINVAL);
}

/**
 *****************************************************************************/

static void rdpdr_fuse_cb_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                             int to_set, struct fuse_file_info *fi)
{
    RDPDR_INODE  *rinode;
    struct stat  st;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d to_set=0x%x", __FUNCTION__, (int) ino, to_set);

    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    if ((rinode = g_rdpdr_fs.inode_table[ino]) == NULL)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "g_rdpdr_fs.inode_table[%d] is NULL", ino);
        fuse_reply_err(req, EBADF);
        return;
    }

    if (to_set & FUSE_SET_ATTR_MODE)
    {
        rinode->mode = attr->st_mode;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_MODE");
    }

    if (to_set & FUSE_SET_ATTR_UID)
    {
        rinode->uid = attr->st_uid;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_UID");
    }

    if (to_set & FUSE_SET_ATTR_GID)
    {
        rinode->gid = attr->st_gid;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_GID");
    }

    if (to_set & FUSE_SET_ATTR_SIZE)
    {
        WLog_Print(g_logger, WLOG_DEBUG, "previous file size: %d", attr->st_size);
        rinode->size = attr->st_size;
        WLog_Print(g_logger, WLOG_DEBUG, "returning file size: %d", rinode->size);
    }

    if (to_set & FUSE_SET_ATTR_ATIME)
    {
        rinode->atime = attr->st_atime;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_ATIME");
    }

    if (to_set & FUSE_SET_ATTR_MTIME)
    {
        rinode->mtime = attr->st_mtime;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_MTIME");
    }

    if (to_set & FUSE_SET_ATTR_ATIME_NOW)
    {
        rinode->atime = attr->st_atime;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_ATIME_NOW");
    }

    if (to_set & FUSE_SET_ATTR_MTIME_NOW)
    {
        rinode->mtime = attr->st_mtime;
        WLog_Print(g_logger, WLOG_DEBUG, "FUSE_SET_ATTR_MTIME_NOW");
    }

    memset(&st, 0, sizeof(st));
    st.st_ino   = rinode->inode;
    st.st_mode  = rinode->mode;
    st.st_size  = rinode->size;
    st.st_uid   = rinode->uid;
    st.st_gid   = rinode->gid;
    st.st_atime = rinode->atime;
    st.st_mtime = rinode->mtime;
    st.st_ctime = rinode->ctime;

    fuse_reply_attr(req, &st, 1.0); /* LK_TODO just faking for now */
}

/**
 * Get dir listing
 *****************************************************************************/
static void rdpdr_fuse_cb_opendir(fuse_req_t req, fuse_ino_t ino,
                             struct fuse_file_info *fi)
{
    struct opendir_req *odreq;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d", __FUNCTION__, (int) ino);

    /* save request */
    odreq = malloc(sizeof(struct opendir_req));
    odreq->req = req;
    odreq->ino = ino;
    odreq->fi  = fi;

    if (Queue_Count(g_opendir_queue) == 0)
    {
        Queue_Enqueue(g_opendir_queue, odreq);
        rdpdr_fuse_proc_opendir_req(req, ino, fi);
    }
    else
    {
        /* place req in queue; rdpdr_fuse_on_drive_query_directory_complete() will handle it */
        Queue_Enqueue(g_opendir_queue, odreq);
    }
}

/**
 * Process the next opendir req
 *
 * @return 0 of the request was sent for remote lookup, -1 otherwise
 *****************************************************************************/
static int
rdpdr_fuse_proc_opendir_req(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    struct dir_info *di;
    RDPDR_INODE     *rinode;
    RDPDR_FUSE_INFO *fip;
    UINT32           device_id;
    char             full_path[4096];
    char            *cptr;

	WLog_Print(g_logger, WLOG_DEBUG, "inode=%d", ino);

    if (!rdpdr_fuse_is_inode_valid(ino))
    {
        WLog_Print(g_logger, WLOG_ERROR, "inode %d is not valid", ino);
        fuse_reply_err(req, EBADF);
        free(Queue_Dequeue(g_opendir_queue));
        return -1;
    }

    if (ino == 1)
        goto done;  /* special case; enumerate top level dir */

    if ((rinode = g_rdpdr_fs.inode_table[ino]) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "g_rdpdr_fs.inode_table[%d] is NULL", ino);
        fuse_reply_err(req, EBADF);
        free(Queue_Dequeue(g_opendir_queue));
        return -1;
    }

    if (rinode->is_loc_resource)
        goto done;

    /* enumerate resources on a remote device */

#ifdef USE_SYNC_FLAG
    if (rinode->is_synced)
    {
        rdpdr_fuse_enum_dir(req, ino, size, off, fi);
        free(Queue_Dequeue(g_opendir_queue));
        return -1;
    }
    else
    {
        goto do_remote_lookup;
    }
#endif

do_remote_lookup:

    rdpdr_fuse_mark_as_stale((int) ino);

    WLog_Print(g_logger, WLOG_DEBUG, "did not find entry; redirecting call to dev_redir");
    device_id = rdpdr_fuse_get_device_id_for_inode((UINT32) ino, full_path);

    WLog_Print(g_logger, WLOG_DEBUG, "dev_id=%d ino=%d full_path=%s", device_id, ino, full_path);

    if ((fip = calloc(1, sizeof(RDPDR_FUSE_INFO))) == NULL)
    {
        WLog_Print(g_logger, WLOG_ERROR, "system out of memory");
        fuse_reply_err(req, ENOMEM);
        free(Queue_Dequeue(g_opendir_queue));
        return -1;
    }

    fip->req = req;
    fip->inode = ino;
    fip->size = 0;
    fip->off = 0;
    fip->fi = fi;
    fip->dirbuf1.first_time = 1;
    fip->dirbuf1.bytes_in_buf = 0;

    fip->invoke_fuse = 1;
    fip->device_id = device_id;

	/* Skip over the root node of the share. */
	if ((cptr = strchr(full_path, '/')) == NULL)
	{
		/* Use the root directory. */
		cptr = "/";
	}

	/* Send a request to query the directory on the client. */
    if (!g_rdpdrServerContext->DriveQueryDirectory(g_rdpdrServerContext, (void *) fip, device_id, cptr))
    {
        WLog_Print(g_logger, WLOG_ERROR, "failed to send drive query directory cmd");
        fuse_reply_buf(req, NULL, 0);
    }

    return 0;

done:

    di = calloc(1, sizeof(struct dir_info));
    di->index = FIRST_INODE;
    fi->fh = (long) di;
    fuse_reply_open(req, fi);
    free(Queue_Dequeue(g_opendir_queue));
    return -1;
}

/**
 *****************************************************************************/

static void
rdpdr_fuse_cb_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    struct dir_info *di;

    WLog_Print(g_logger, WLOG_DEBUG, "%s: ino=%d", __FUNCTION__, (int) ino);

    di = (struct dir_info *) (LONG_PTR) fi->fh;
    if (di)
        free(di);

    fuse_reply_err(req, 0);
}

/******************************************************************************
 *                           miscellaneous functions
 *****************************************************************************/

/**
 * Mark all files with matching parent inode as stale
 *
 * @param  pinode  the parent inode
 *****************************************************************************/

static void
rdpdr_fuse_mark_as_stale(int pinode)
{
    int          i;
    RDPDR_INODE *rinode;

    if ((pinode < FIRST_INODE) || (pinode >=  g_rdpdr_fs.num_entries))
        return;

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* match parent inode */
        if (rinode->parent_inode != pinode)
            continue;

        /* got a match */
        rinode->stale = 1;
    }
}

/**
 * Delete all files with matching parent inode that are marked as stale
 *
 * @param  pinode  the parent inode
 *****************************************************************************/

static void
rdpdr_fuse_delete_stale_entries(int pinode)
{
    int          i;
    RDPDR_INODE *rinode;

    if ((pinode < FIRST_INODE) || (pinode >=  g_rdpdr_fs.num_entries))
        return;

    for (i = FIRST_INODE; i < g_rdpdr_fs.num_entries; i++)
    {
        if ((rinode = g_rdpdr_fs.inode_table[i]) == NULL)
            continue;

        /* match parent inode */
        if (rinode->parent_inode != pinode)
            continue;

        /* got a match, but is it stale? */
        if (!rinode->stale)
            continue;

        /* ok to delete */
        if (rinode->mode & S_IFREG)
            rdpdr_fuse_delete_file_with_rinode(rinode);
        else
            rdpdr_fuse_recursive_delete_dir_with_rinode(rinode);
    }
}

#endif /* end else #ifndef RDPDR_FUSE */
