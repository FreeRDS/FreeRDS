/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
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
 *
 * generic operating system calls
 */

#if !defined(OS_CALLS_H)
#define OS_CALLS_H

#ifndef NULL
#define NULL 0
#endif

#include <winpr/wtypes.h>

int g_rm_temp_dir(void);
int g_mk_temp_dir(const char* app_name);
void g_init(const char* app_name);
void g_deinit(void);
void* g_malloc(int size, int zero);
void g_sprintf(char* dest, const char* format, ...);
void g_snprintf(char* dest, int len, const char* format, ...);
int g_tcp_set_no_delay(int sck);
int g_tcp_set_keepalive(int sck);
int g_tcp_socket(void);
int g_tcp_local_socket(void);
void g_tcp_close(int sck);
int g_tcp_connect(int sck, const char* address, const char* port);
int g_tcp_local_connect(int sck, const char* port);
int g_tcp_set_non_blocking(int sck);
int g_tcp_bind(int sck, const char *port);
int g_tcp_local_bind(int sck, const char* port);
int g_tcp_bind_address(int sck, const char* port, const char* address);
int g_tcp_listen(int sck);
int g_tcp_accept(int sck);
int g_tcp_recv(int sck, void* ptr, int len, int flags);
int g_tcp_send(int sck, const void* ptr, int len, int flags);
int g_tcp_last_error_would_block(int sck);
int g_tcp_socket_ok(int sck);
int g_tcp_can_send(int sck, int millis);
int g_tcp_can_recv(int sck, int millis);
int g_tcp_select(int sck1, int sck2);
void g_write_ip_address(int rcv_sck, char* ip_address, int bytes);
void g_sleep(int msecs);
int g_file_open(const char* file_name);
int g_file_open_ex(const char *file_name, int aread, int awrite, int acreate, int atrunc);
int g_file_close(int fd);
int g_file_read(int fd, unsigned char* ptr, int len);
int g_file_write(int fd, unsigned char* ptr, int len);
int g_file_seek(int fd, int offset);
int g_file_lock(int fd, int start, int len);
int g_chmod_hex(const char* filename, int flags);
int g_mkdir(const char* dirname);
char* g_get_current_dir(char* dirname, int maxlen);
int g_set_current_dir(char* dirname);
int g_file_exist(const char* filename);
int g_directory_exist(const char* dirname);
int g_create_dir(const char* dirname);
int g_create_path(const char* path);
int g_remove_dir(const char* dirname);
int g_file_delete(const char* filename);
int g_file_get_size(const char* filename);
void g_signal_user_interrupt(void (*func)(int));
void g_signal_kill(void (*func)(int));
void g_signal_terminate(void (*func)(int));
void g_signal_pipe(void (*func)(int));
int g_fork(void);
int g_setgid(int pid);
int g_initgroups(const char* user, int gid);
int g_getuid(void);
int g_getgid(void);
int g_setuid(int pid);
int g_exit(int exit_code);
int g_getpid(void);
int g_sigterm(int pid);
int g_getuser_info(const char* username, int* gid, int* uid, char* shell, char* dir, char* gecos);
int g_getgroup_info(const char* groupname, int* gid);
int g_check_user_in_group(const char* username, int gid, int* ok);
int g_tcp_local_socket_dgram(void);
int g_tcp_local_socket_stream(void);
int g_tcp_select2(int sck1, int sck2);
int g_tcp_select3(int sck1, int sck2, int sck3);

#endif
