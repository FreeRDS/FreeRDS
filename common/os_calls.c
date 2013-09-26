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
 *
 * put all the os / arch define in here you want
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#include <winsock.h>
#else
/* fix for solaris 10 with gcc 3.3.2 problem */
#if defined(sun) || defined(__sun)
#define ctid_t id_t
#endif
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <locale.h>

#include "os_calls.h"

/* for clearenv() */
#if defined(_WIN32)
#else
extern char **environ;
#endif

#if defined(__linux__)
#include <linux/unistd.h>
#endif

/* for solaris */
#if !defined(PF_LOCAL)
#define PF_LOCAL AF_UNIX
#endif
#if !defined(INADDR_NONE)
#define INADDR_NONE ((unsigned long)-1)
#endif

static char g_temp_base[128] = "";
static char g_temp_base_org[128] = "";

int g_rm_temp_dir(void)
{
	if (g_temp_base[0] != 0)
	{
		if (!g_remove_dir(g_temp_base))
		{
			printf("g_rm_temp_dir: removing temp directory [%s] failed\n", g_temp_base);
		}

		g_temp_base[0] = 0;
	}

	return 0;
}

int g_mk_temp_dir(const char *app_name)
{
	if (app_name != 0)
	{
		if (app_name[0] != 0)
		{
			if (!g_directory_exist("/tmp/.xrdp"))
			{
				if (!g_create_dir("/tmp/.xrdp"))
				{
					printf("g_mk_temp_dir: g_create_dir failed\n");
					return 1;
				}

				g_chmod_hex("/tmp/.xrdp", 0x1777);
			}

			snprintf(g_temp_base, sizeof(g_temp_base), "/tmp/.xrdp/%s-XXXXXX", app_name);
			snprintf(g_temp_base_org, sizeof(g_temp_base_org), "/tmp/.xrdp/%s-XXXXXX", app_name);

			if (mkdtemp(g_temp_base) == 0)
			{
				printf("g_mk_temp_dir: mkdtemp failed [%s]\n", g_temp_base);
				return 1;
			}
		}
		else
		{
			printf("g_mk_temp_dir: bad app name\n");
			return 1;
		}
	}
	else
	{
		if (g_temp_base_org[0] == 0)
		{
			printf("g_mk_temp_dir: g_temp_base_org not set\n");
			return 1;
		}

		strncpy(g_temp_base, g_temp_base_org, 127);

		if (mkdtemp(g_temp_base) == 0)
		{
			printf("g_mk_temp_dir: mkdtemp failed [%s]\n", g_temp_base);
		}
	}

	return 0;
}

void g_init(const char *app_name)
{
#if defined(_WIN32)
	WSADATA wsadata;

	WSAStartup(2, &wsadata);
#endif
	setlocale(LC_CTYPE, "");
	g_mk_temp_dir(app_name);
}

void g_deinit(void)
{
#if defined(_WIN32)
	WSACleanup();
#endif
	g_rm_temp_dir();
}

/* allocate memory, returns a pointer to it, size bytes are allocated,
 if zero is non zero, each byte will be set to zero */
void* g_malloc(int size, int zero)
{
	char *rv;

	rv = (char*) malloc(size);

	if (zero)
	{
		if (rv != 0)
		{
			memset(rv, 0, size);
		}
	}

	return rv;
}

void g_sprintf(char *dest, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsprintf(dest, format, ap);
	va_end(ap);
}

void g_snprintf(char *dest, int len, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsnprintf(dest, len, format, ap);
	va_end(ap);
}

/*Returns 0 on success*/
int g_tcp_set_no_delay(int sck)
{
	int ret = 1; /* error */
#if defined(_WIN32)
	int option_value;
	int option_len;
#else
	int option_value;
	unsigned int option_len;
#endif

	option_len = sizeof(option_value);

	/* SOL_TCP IPPROTO_TCP */
	if (getsockopt(sck, IPPROTO_TCP, TCP_NODELAY, (char *) &option_value, &option_len) == 0)
	{
		if (option_value == 0)
		{
			option_value = 1;
			option_len = sizeof(option_value);

			if (setsockopt(sck, IPPROTO_TCP, TCP_NODELAY, (char *) &option_value, option_len) == 0)
			{
				ret = 0; /* success */
			}
			else
			{
				printf("Error setting tcp_nodelay\n");
			}
		}
	}
	else
	{
		printf("Error getting tcp_nodelay\n");
	}

	return ret;
}

/*Returns 0 on success*/
int g_tcp_set_keepalive(int sck)
{
	int ret = 1; /* error */
#if defined(_WIN32)
	int option_value;
	int option_len;
#else
	int option_value;
	unsigned int option_len;
#endif

	option_len = sizeof(option_value);

	/* SOL_TCP IPPROTO_TCP */
	if (getsockopt(sck, SOL_SOCKET, SO_KEEPALIVE, (char *) &option_value, &option_len) == 0)
	{
		if (option_value == 0)
		{
			option_value = 1;
			option_len = sizeof(option_value);

			if (setsockopt(sck, SOL_SOCKET, SO_KEEPALIVE, (char *) &option_value, option_len) == 0)
			{
				ret = 0; /* success */
			}
			else
			{
				printf("Error setting tcp_keepalive\n");
			}
		}
	}
	else
	{
		printf("Error getting tcp_keepalive\n");
	}

	return ret;
}

/* returns a newly created socket or -1 on error */
/* in win32 a socket is an unsigned int, in linux, its an int */
int g_tcp_socket(void)
{
	int rv;
	int option_value;
#if defined(_WIN32)
	int option_len;
#else
	unsigned int option_len;
#endif

#if !defined(NO_ARPA_INET_H_IP6)
	rv = (int) socket(AF_INET6, SOCK_STREAM, 0);
#else
	rv = (int)socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (rv < 0)
	{
		return -1;
	}
#if !defined(NO_ARPA_INET_H_IP6)
	option_len = sizeof(option_value);
	if (getsockopt(rv, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &option_value, &option_len) == 0)
	{
		if (option_value != 0)
		{
			option_value = 0;
			option_len = sizeof(option_value);
			setsockopt(rv, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &option_value, option_len);
		}
	}
#endif
	option_len = sizeof(option_value);
	if (getsockopt(rv, SOL_SOCKET, SO_REUSEADDR, (char *) &option_value, &option_len) == 0)
	{
		if (option_value == 0)
		{
			option_value = 1;
			option_len = sizeof(option_value);
			setsockopt(rv, SOL_SOCKET, SO_REUSEADDR, (char *) &option_value, option_len);
		}
	}

	option_len = sizeof(option_value);

	if (getsockopt(rv, SOL_SOCKET, SO_SNDBUF, (char *) &option_value, &option_len) == 0)
	{
		if (option_value < (1024 * 32))
		{
			option_value = 1024 * 32;
			option_len = sizeof(option_value);
			setsockopt(rv, SOL_SOCKET, SO_SNDBUF, (char *) &option_value, option_len);
		}
	}

	return rv;
}

int g_tcp_local_socket(void)
{
#if defined(_WIN32)
	return 0;
#else
	return socket(PF_LOCAL, SOCK_STREAM, 0);
#endif
}

void g_tcp_close(int sck)
{
	char ip[256];

	if (sck == 0)
	{
		return;
	}
#if defined(_WIN32)
	closesocket(sck);
#else
	g_write_ip_address(sck, ip, 255);
	close(sck);
#endif
}

/* returns error, zero is good */
int g_tcp_connect(int sck, const char *address, const char *port)
{
	int res = 0;
	struct addrinfo p;
	struct addrinfo *h = (struct addrinfo *) NULL;
	struct addrinfo *rp = (struct addrinfo *) NULL;

	/* initialize (zero out) local variables: */
	memset(&p, 0, sizeof(struct addrinfo));

	/* in IPv6-enabled environments, set the AI_V4MAPPED
	 * flag in ai_flags and specify ai_family=AF_INET6 in
	 * order to ensure that getaddrinfo() returns any
	 * available IPv4-mapped addresses in case the target
	 * host does not have a true IPv6 address:
	 */
	p.ai_socktype = SOCK_STREAM;
	p.ai_protocol = IPPROTO_TCP;
#if !defined(NO_ARPA_INET_H_IP6)
	p.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
	p.ai_family = AF_INET6;
	if (strcmp(address, "127.0.0.1") == 0)
	{
		res = getaddrinfo("::1", port, &p, &h);
	} else
	{
		res = getaddrinfo(address, port, &p, &h);
	}
#else
	p.ai_flags = AI_ADDRCONFIG;
	p.ai_family = AF_INET;
	res = getaddrinfo(address, port, &p, &h);
#endif
	if (res > -1)
	{
		if (h != NULL)
		{
			for (rp = h; rp != NULL; rp = rp->ai_next)
			{
				rp = h;
				res = connect(sck, (struct sockaddr *) (rp->ai_addr), rp->ai_addrlen);
				if (res != -1)
				{
					break; /* Success */
				}
			}
		}
	}
	return res;
}

/* returns error, zero is good */
int g_tcp_local_connect(int sck, const char *port)
{
#if defined(_WIN32)
	return -1;
#else
	struct sockaddr_un s;

	memset(&s, 0, sizeof(struct sockaddr_un));
	s.sun_family = AF_UNIX;
	strcpy(s.sun_path, port);
	return connect(sck, (struct sockaddr *) &s, sizeof(struct sockaddr_un));
#endif
}

int g_tcp_set_non_blocking(int sck)
{
	unsigned long i;

#if defined(_WIN32)
	i = 1;
	ioctlsocket(sck, FIONBIO, &i);
#else
	i = fcntl(sck, F_GETFL);
	i = i | O_NONBLOCK;
	fcntl(sck, F_SETFL, i);
#endif
	return 0;
}

/* return boolean */
static int address_match(const char *address, struct addrinfo *j)
{
	struct sockaddr_in* ipv4_in;
	struct sockaddr_in6* ipv6_in;

	if (!address)
		return 1;

	if (strlen(address) < 1)
		return 1;

	if (strcmp(address, "0.0.0.0") == 0)
		return 1;

	if ((strcmp(address, "127.0.0.1") == 0) ||
			(strcmp(address, "::1") == 0) || (strcmp(address, "localhost") == 0))
	{
		if (j->ai_addr != 0)
		{
			if (j->ai_addr->sa_family == AF_INET)
			{
				ipv4_in = (struct sockaddr_in*) (j->ai_addr);

				if (inet_pton(AF_INET, "127.0.0.1", &(ipv4_in->sin_addr)))
					return 1;
			}

			if (j->ai_addr->sa_family == AF_INET6)
			{
				ipv6_in = (struct sockaddr_in6*) (j->ai_addr);

				if (inet_pton(AF_INET6, "::1", &(ipv6_in->sin6_addr)))
					return 1;
			}
		}
	}

	if (j->ai_addr != 0)
	{
		if (j->ai_addr->sa_family == AF_INET)
		{
			ipv4_in = (struct sockaddr_in*) (j->ai_addr);

			if (inet_pton(AF_INET, address, &(ipv4_in->sin_addr)))
				return 1;
		}

		if (j->ai_addr->sa_family == AF_INET6)
		{
			ipv6_in = (struct sockaddr_in6*) (j->ai_addr);

			if (inet_pton(AF_INET6, address, &(ipv6_in->sin6_addr)))
				return 1;
		}
	}

	return 0;
}

/* returns error, zero is good */
static int  g_tcp_bind_flags(int sck, const char* port, const char* address, int flags)
{
	int error;
	int status;
	struct addrinfo* res;
	struct addrinfo hints = { 0 };

	status = -1;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = flags;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	error = getaddrinfo(NULL, port, &hints, &res);

	if (error == 0)
	{
		while ((res != 0) && (status < 0))
		{
			if (address_match(address, res))
			{
				status = bind(sck, res->ai_addr, res->ai_addrlen);
			}

			res = res->ai_next;
		}
	}

	return status;
}

/* returns error, zero is good */
int g_tcp_bind(int sck, const char *port)
{
	int flags;

	flags = AI_ADDRCONFIG | AI_PASSIVE;
	return g_tcp_bind_flags(sck, port, 0, flags);
}

int g_tcp_local_bind(int sck, const char *port)
{
#if defined(_WIN32)
	return -1;
#else
	struct sockaddr_un s;

	memset(&s, 0, sizeof(struct sockaddr_un));
	s.sun_family = AF_UNIX;
	strcpy(s.sun_path, port);
	return bind(sck, (struct sockaddr *) &s, sizeof(struct sockaddr_un));
#endif
}

/* returns error, zero is good */
int g_tcp_bind_address(int sck, const char *port, const char *address)
{
	int flags;

	flags = AI_ADDRCONFIG | AI_PASSIVE;
	return g_tcp_bind_flags(sck, port, address, flags);
}

/* returns error, zero is good */
int g_tcp_listen(int sck)
{
	return listen(sck, 2);
}

int g_tcp_accept(int sck)
{
	int ret;
	char ipAddr[256];
	struct sockaddr_in s;
#if defined(_WIN32)
	signed int i;
#else
	unsigned int i;
#endif

	i = sizeof(struct sockaddr_in);
	memset(&s, 0, i);
	ret = accept(sck, (struct sockaddr *) &s, &i);
	if (ret > 0)
	{
		snprintf(ipAddr, 255, "A connection received from: %s port %d", inet_ntoa(s.sin_addr),
				ntohs(s.sin_port));
	}
	return ret;
}

void g_write_ip_address(int rcv_sck, char *ip_address, int bytes)
{
	struct sockaddr_in s;
	struct in_addr in;
#if defined(_WIN32)
	int len;
#else
	unsigned int len;
#endif
	int ip_port;
	int ok;

	ok = 0;
	memset(&s, 0, sizeof(s));
	len = sizeof(s);

	if (getpeername(rcv_sck, (struct sockaddr *) &s, &len) == 0)
	{
		memset(&in, 0, sizeof(in));
		in.s_addr = s.sin_addr.s_addr;
		ip_port = ntohs(s.sin_port);

		if (ip_port != 0)
		{
			ok = 1;
			snprintf(ip_address, bytes, "%s:%d - socket: %d", inet_ntoa(in), ip_port, rcv_sck);
		}
	}

	if (!ok)
	{
		snprintf(ip_address, bytes, "NULL:NULL - socket: %d", rcv_sck);
	}
}

void g_sleep(int msecs)
{
#if defined(_WIN32)
	Sleep(msecs);
#else
	usleep(msecs * 1000);
#endif
}

int g_tcp_last_error_would_block(int sck)
{
#if defined(_WIN32)
	return WSAGetLastError() == WSAEWOULDBLOCK;
#else
	return (errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINPROGRESS);
#endif
}

int g_tcp_recv(int sck, void *ptr, int len, int flags)
{
#if defined(_WIN32)
	return recv(sck, (char *)ptr, len, flags);
#else
	return recv(sck, ptr, len, flags);
#endif
}

int g_tcp_send(int sck, const void *ptr, int len, int flags)
{
#if defined(_WIN32)
	return send(sck, (const char *)ptr, len, flags);
#else
	return send(sck, ptr, len, flags);
#endif
}

/* returns boolean */
int g_tcp_socket_ok(int sck)
{
#if defined(_WIN32)
	int opt;
	int opt_len;
#else
	int opt;
	unsigned int opt_len;
#endif

	opt_len = sizeof(opt);

	if (getsockopt(sck, SOL_SOCKET, SO_ERROR, (char *) (&opt), &opt_len) == 0)
	{
		if (opt == 0)
		{
			return 1;
		}
	}

	return 0;
}

/* wait 'millis' milliseconds for the socket to be able to write */
/* returns boolean */
int g_tcp_can_send(int sck, int millis)
{
	fd_set wfds;
	struct timeval time;
	int rv;

	time.tv_sec = millis / 1000;
	time.tv_usec = (millis * 1000) % 1000000;
	FD_ZERO(&wfds);

	if (sck > 0)
	{
		FD_SET(((unsigned int)sck), &wfds);
		rv = select(sck + 1, 0, &wfds, 0, &time);

		if (rv > 0)
		{
			return g_tcp_socket_ok(sck);
		}
	}

	return 0;
}

/* wait 'millis' milliseconds for the socket to be able to receive */
/* returns boolean */
int g_tcp_can_recv(int sck, int millis)
{
	fd_set rfds;
	struct timeval time;
	int rv;

	time.tv_sec = millis / 1000;
	time.tv_usec = (millis * 1000) % 1000000;
	FD_ZERO(&rfds);

	if (sck > 0)
	{
		FD_SET(((unsigned int)sck), &rfds);
		rv = select(sck + 1, &rfds, 0, 0, &time);

		if (rv > 0)
		{
			return g_tcp_socket_ok(sck);
		}
	}

	return 0;
}

int g_tcp_select(int sck1, int sck2)
{
	fd_set rfds;
	struct timeval time;
	int max = 0;
	int rv = 0;

	memset(&rfds, 0, sizeof(fd_set));
	memset(&time, 0, sizeof(struct timeval));

	time.tv_sec = 0;
	time.tv_usec = 0;
	FD_ZERO(&rfds);

	if (sck1 > 0)
	{
		FD_SET(((unsigned int)sck1), &rfds);
	}

	if (sck2 > 0)
	{
		FD_SET(((unsigned int)sck2), &rfds);
	}

	max = sck1;

	if (sck2 > max)
	{
		max = sck2;
	}

	rv = select(max + 1, &rfds, 0, 0, &time);

	if (rv > 0)
	{
		rv = 0;

		if (FD_ISSET(((unsigned int)sck1), &rfds))
		{
			rv = rv | 1;
		}

		if (FD_ISSET(((unsigned int)sck2), &rfds))
		{
			rv = rv | 2;
		}
	} else
	{
		rv = 0;
	}

	return rv;
}

/* returns -1 on error, else return handle or file descriptor */
int g_file_open(const char *file_name)
{
#if defined(_WIN32)
	return (int)CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#else
	int rv;

	rv = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (rv == -1)
	{
		/* can't open read / write, try to open read only */
		rv = open(file_name, O_RDONLY);
	}

	return rv;
#endif
}

/* returns -1 on error, else return handle or file descriptor */
int g_file_open_ex(const char *file_name, int aread, int awrite, int acreate, int atrunc)
{
#if defined(_WIN32)
	return -1;
#else
	int rv;
	int flags;

	flags = 0;

	if (aread && awrite)
	{
		flags |= O_RDWR;
	}
	else if (aread)
	{
		flags |= O_RDONLY;
	}
	else if (awrite)
	{
		flags |= O_WRONLY;
	}

	if (acreate)
	{
		flags |= O_CREAT;
	}

	if (atrunc)
	{
		flags |= O_TRUNC;
	}

	rv = open(file_name, flags, S_IRUSR | S_IWUSR);
	return rv;
#endif
}

/* returns error, always 0 */
int g_file_close(int fd)
{
#if defined(_WIN32)
	CloseHandle((HANDLE)fd);
#else
	close(fd);
#endif
	return 0;
}

/* read from file, returns the number of bytes read or -1 on error */
int g_file_read(int fd, unsigned char* ptr, int len)
{
#if defined(_WIN32)

	if (ReadFile((HANDLE)fd, (LPVOID)ptr, (DWORD)len, (LPDWORD)&len, 0))
	{
		return len;
	}
	else
	{
		return -1;
	}

#else
	return read(fd, ptr, len);
#endif
}

/* write to file, returns the number of bytes writen or -1 on error */
int g_file_write(int fd, unsigned char* ptr, int len)
{
#if defined(_WIN32)

	if (WriteFile((HANDLE)fd, (LPVOID)ptr, (DWORD)len, (LPDWORD)&len, 0))
	{
		return len;
	}
	else
	{
		return -1;
	}

#else
	return write(fd, ptr, len);
#endif
}

/* move file pointer, returns offset on success, -1 on failure */
int g_file_seek(int fd, int offset)
{
#if defined(_WIN32)
	int rv;

	rv = (int)SetFilePointer((HANDLE)fd, offset, 0, FILE_BEGIN);

	if (rv == (int)INVALID_SET_FILE_POINTER)
	{
		return -1;
	}
	else
	{
		return rv;
	}

#else
	return (int) lseek(fd, offset, SEEK_SET);
#endif
}

/* do a write lock on a file */
/* return boolean */
int g_file_lock(int fd, int start, int len)
{
#if defined(_WIN32)
	return LockFile((HANDLE)fd, start, 0, len, 0);
#else
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = start;
	lock.l_len = len;

	if (fcntl(fd, F_SETLK, &lock) == -1)
	{
		return 0;
	}

	return 1;
#endif
}

/* returns error */
int g_chmod_hex(const char *filename, int flags)
{
#if defined(_WIN32)
	return 0;
#else
	int fl;

	fl = 0;
	fl |= (flags & 0x4000) ? S_ISUID : 0;
	fl |= (flags & 0x2000) ? S_ISGID : 0;
	fl |= (flags & 0x1000) ? S_ISVTX : 0;
	fl |= (flags & 0x0400) ? S_IRUSR : 0;
	fl |= (flags & 0x0200) ? S_IWUSR : 0;
	fl |= (flags & 0x0100) ? S_IXUSR : 0;
	fl |= (flags & 0x0040) ? S_IRGRP : 0;
	fl |= (flags & 0x0020) ? S_IWGRP : 0;
	fl |= (flags & 0x0010) ? S_IXGRP : 0;
	fl |= (flags & 0x0004) ? S_IROTH : 0;
	fl |= (flags & 0x0002) ? S_IWOTH : 0;
	fl |= (flags & 0x0001) ? S_IXOTH : 0;
	return chmod(filename, fl);
#endif
}

/* returns error, always zero */
int g_mkdir(const char *dirname)
{
#if defined(_WIN32)
	return 0;
#else
	mkdir(dirname, S_IRWXU);
	return 0;
#endif
}

/* gets the current working directory and puts up to maxlen chars in
 dirname
 always returns 0 */
char* g_get_current_dir(char *dirname, int maxlen)
{
#if defined(_WIN32)
	GetCurrentDirectoryA(maxlen, dirname);
	return 0;
#else

	if (getcwd(dirname, maxlen) == 0)
	{
	}

	return 0;
#endif
}

/* returns error, zero on success and -1 on failure */
int g_set_current_dir(char *dirname)
{
#if defined(_WIN32)

	if (SetCurrentDirectoryA(dirname))
	{
		return 0;
	}
	else
	{
		return -1;
	}

#else
	return chdir(dirname);
#endif
}

/* returns boolean, non zero if the file exists */
int g_file_exist(const char *filename)
{
#if defined(_WIN32)
	return 0; // use FileAge(filename) <> -1
#else
	return access(filename, F_OK) == 0;
#endif
}

/* returns boolean, non zero if the directory exists */
int g_directory_exist(const char *dirname)
{
#if defined(_WIN32)
	return 0; // use GetFileAttributes and check return value
	// is not -1 and FILE_ATTRIBUT_DIRECTORY bit is set
#else
	struct stat st;

	if (stat(dirname, &st) == 0)
	{
		return S_ISDIR(st.st_mode);
	}
	else
	{
		return 0;
	}

#endif
}

/* returns boolean */
int g_create_dir(const char *dirname)
{
#if defined(_WIN32)
	return CreateDirectoryA(dirname, 0); // test this
#else
	return mkdir(dirname, (mode_t) -1) == 0;
#endif
}

/* will try to create directories up to last / in name
 example /tmp/a/b/c/readme.txt will try to create /tmp/a/b/c
 returns boolean */
int g_create_path(const char *path)
{
	char *pp;
	char *sp;
	char *copypath;
	int status;

	status = 1;
	copypath = _strdup(path);
	pp = copypath;
	sp = strchr(pp, '/');

	while (sp != 0)
	{
		if (sp != pp)
		{
			*sp = 0;

			if (!g_directory_exist(copypath))
			{
				if (!g_create_dir(copypath))
				{
					status = 0;
					break;
				}
			}

			*sp = '/';
		}

		pp = sp + 1;
		sp = strchr(pp, '/');
	}

	free(copypath);
	return status;
}

/* returns boolean */
int g_remove_dir(const char *dirname)
{
#if defined(_WIN32)
	return RemoveDirectoryA(dirname); // test this
#else
	return rmdir(dirname) == 0;
#endif
}

/* returns non zero if the file was deleted */
int g_file_delete(const char *filename)
{
#if defined(_WIN32)
	return DeleteFileA(filename);
#else
	return unlink(filename) != -1;
#endif
}

/* returns file size, -1 on error */
int g_file_get_size(const char *filename)
{
#if defined(_WIN32)
	return -1;
#else
	struct stat st;

	if (stat(filename, &st) == 0)
	{
		return (int) (st.st_size);
	}
	else
	{
		return -1;
	}

#endif
}

/* does not work in win32 */
void g_signal_user_interrupt(void(*func)(int))
{
#if defined(_WIN32)
#else
	signal(SIGINT, func);
#endif
}

/* does not work in win32 */
void g_signal_kill(void(*func)(int))
{
#if defined(_WIN32)
#else
	signal(SIGKILL, func);
#endif
}

/* does not work in win32 */
void g_signal_terminate(void(*func)(int))
{
#if defined(_WIN32)
#else
	signal(SIGTERM, func);
#endif
}

/* does not work in win32 */
void g_signal_pipe(void(*func)(int))
{
#if defined(_WIN32)
#else
	signal(SIGPIPE, func);
#endif
}

/* does not work in win32 */
int g_fork(void)
{
#if defined(_WIN32)
	return 0;
#else
	int rv;

	rv = fork();

	if (rv == 0) /* child */
	{
		g_mk_temp_dir(0);
	}

	return rv;
#endif
}

/* does not work in win32 */
int g_setgid(int pid)
{
#if defined(_WIN32)
	return 0;
#else
	return setgid(pid);
#endif
}

/* returns error, zero is success, non zero is error */
/* does not work in win32 */
int g_initgroups(const char *user, int gid)
{
#if defined(_WIN32)
	return 0;
#else
	return initgroups(user, gid);
#endif
}

/* does not work in win32 */
/* returns user id */
int g_getuid(void)
{
#if defined(_WIN32)
	return 0;
#else
	return getuid();
#endif
}

/* does not work in win32 */
/* returns user id */
int g_getgid(void)
{
#if defined(_WIN32)
	return 0;
#else
	return getgid();
#endif
}

/* does not work in win32 */
/* On success, zero is returned. On error, -1 is returned */
int g_setuid(int pid)
{
#if defined(_WIN32)
	return 0;
#else
	return setuid(pid);
#endif
}

int g_exit(int exit_code)
{
	_exit(exit_code);
	return 0;
}

int g_getpid(void)
{
#if defined(_WIN32)
	return (int)GetCurrentProcessId();
#else
	return (int) getpid();
#endif
}

/* does not work in win32 */
int g_sigterm(int pid)
{
#if defined(_WIN32)
	return 0;
#else
	return kill(pid, SIGTERM);
#endif
}

/* returns 0 if ok */
/* does not work in win32 */
int g_getuser_info(const char *username, int *gid, int *uid, char *shell, char *dir, char *gecos)
{
#if defined(_WIN32)
	return 1;
#else
	struct passwd *pwd_1;

	pwd_1 = getpwnam(username);

	if (pwd_1 != 0)
	{
		if (gid != 0)
		{
			*gid = pwd_1->pw_gid;
		}

		if (uid != 0)
		{
			*uid = pwd_1->pw_uid;
		}

		if (dir != 0)
		{
			strcpy(dir, pwd_1->pw_dir);
		}

		if (shell != 0)
		{
			strcpy(shell, pwd_1->pw_shell);
		}

		if (gecos != 0)
		{
			strcpy(gecos, pwd_1->pw_gecos);
		}

		return 0;
	}

	return 1;
#endif
}

/* returns 0 if ok */
/* does not work in win32 */
int g_getgroup_info(const char *groupname, int *gid)
{
#if defined(_WIN32)
	return 1;
#else
	struct group *g;

	g = getgrnam(groupname);

	if (g != 0)
	{
		if (gid != 0)
		{
			*gid = g->gr_gid;
		}

		return 0;
	}

	return 1;
#endif
}

/* returns error */
/* if zero is returned, then ok is set */
/* does not work in win32 */
int g_check_user_in_group(const char *username, int gid, int *ok)
{
#if defined(_WIN32)
	return 1;
#else
	struct group *groups;
	int i;

	groups = getgrgid(gid);

	if (groups == 0)
	{
		return 1;
	}

	*ok = 0;
	i = 0;

	while (0 != groups->gr_mem[i])
	{
		if (0 == strcmp(groups->gr_mem[i], username))
		{
			*ok = 1;
			break;
		}

		i++;
	}

	return 0;
#endif
}

int g_tcp_local_socket_dgram(void)
{
	return socket(AF_UNIX, SOCK_DGRAM, 0);
}

int g_tcp_local_socket_stream(void)
{
	return socket(AF_UNIX, SOCK_STREAM, 0);
}

int g_tcp_select2(int sck1, int sck2)
{
	fd_set rfds;
	struct timeval time;
	int max;
	int rv;

	time.tv_sec = 0;
	time.tv_usec = 0;
	FD_ZERO(&rfds);

	if (sck1 > 0)
	{
		FD_SET(((unsigned int)sck1), &rfds);
	}

	if (sck2 > 0)
	{
		FD_SET(((unsigned int)sck2), &rfds);
	}

	max = sck1;

	if (sck2 > max)
	{
		max = sck2;
	}

	rv = select(max + 1, &rfds, 0, 0, &time);

	if (rv > 0)
	{
		rv = 0;

		if (FD_ISSET(((unsigned int)sck1), &rfds))
		{
			rv = rv | 1;
		}

		if (FD_ISSET(((unsigned int)sck2), &rfds))
		{
			rv = rv | 2;
		}
	}
	else
	{
		rv = 0;
	}

	return rv;
}

int g_tcp_select3(int sck1, int sck2, int sck3)
{
	fd_set rfds;
	struct timeval time;
	int max;
	int rv;

	time.tv_sec = 0;
	time.tv_usec = 0;
	FD_ZERO(&rfds);

	if (sck1 > 0)
	{
		FD_SET(((unsigned int)sck1), &rfds);
	}

	if (sck2 > 0)
	{
		FD_SET(((unsigned int)sck2), &rfds);
	}

	if (sck3 > 0)
	{
		FD_SET(((unsigned int)sck3), &rfds);
	}

	max = sck1;

	if (sck2 > max)
	{
		max = sck2;
	}

	if (sck3 > max)
	{
		max = sck3;
	}

	rv = select(max + 1, &rfds, 0, 0, &time);

	if (rv > 0)
	{
		rv = 0;

		if (FD_ISSET(((unsigned int)sck1), &rfds))
		{
			rv = rv | 1;
		}

		if (FD_ISSET(((unsigned int)sck2), &rfds))
		{
			rv = rv | 2;
		}

		if (FD_ISSET(((unsigned int)sck3), &rfds))
		{
			rv = rv | 4;
		}
	}
	else
	{
		rv = 0;
	}

	return rv;
}
