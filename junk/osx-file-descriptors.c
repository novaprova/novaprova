#include <sys/types.h>
#include <stdio.h>
#include <memory.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef INADDR_LOCALHOST
#define INADDR_LOCALHOST    htonl(0x7f000001)
#endif

static void describe_sockaddr(const struct sockaddr *sa)
{
    switch (sa->sa_family)
    {
    case AF_INET:
	{
	    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	    char buf[INET_ADDRSTRLEN];
	    printf("inet %s:%d",
		   inet_ntop(sin->sin_family,
			     &sin->sin_addr.s_addr,
			     buf, sizeof(buf)),
		   ntohs(sin->sin_port));
	}
	break;
    case AF_INET6:
	{
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
	    char buf[INET6_ADDRSTRLEN];
	    printf("inet6 %s.%d",
		   inet_ntop(sin6->sin6_family,
			     &sin6->sin6_addr,
			     buf, sizeof(buf)),
		   ntohs(sin6->sin6_port));
	}
	break;
    /*
    case AF_UNIX:
    */
    default:
	printf("unexpected socket family %d", sa->sa_family);
	break;
    }
}

static void describe_fd(int fd)
{
    struct stat sb;
    int r;
    bool need_path = false;
    bool need_sockname = false;

    r = fstat(fd, &sb);
    if (r < 0)
    {
	if (errno == EBADF)
	{
	    printf("%d: not a file descriptor\n", fd);
	    return;
	}
	if (errno == EACCES || errno == EPERM)
	{
	    printf("%d: permission denied\n", fd);
	    return;
	}
	perror("fstat");
	return;
    }
    switch (sb.st_mode & S_IFMT)
    {
    case S_IFIFO:   /* named pipe (fifo) or actual pipe */
	/* Experiment indicates named pipes have non-zero st_dev
	 * field but anonymous pipes don't */
	if (sb.st_dev == 0)
	{
	    printf("%d: pipe:0x%llx\n", fd, sb.st_ino);
	}
	else
	{
	    need_path = true;
	}
	break;
    case S_IFCHR:   /* character special */
	printf("%d: char device %d:%d\n", fd, major(sb.st_rdev), minor(sb.st_rdev));
	return;
    case S_IFDIR:   /* directory */
	need_path = true;
	printf("%d: directory ", fd);
	break;
    case S_IFBLK:   /* block special */
	printf("%d: block device %d:%d\n", fd, major(sb.st_rdev), minor(sb.st_rdev));
	return;
    case S_IFREG:   /* regular file */
	need_path = true;
	printf("%d: file ", fd);
	break;
    /* We won't get S_IFLNK from fstat(), any symlink
     * has already been followed at open time */
    case S_IFSOCK:  /* socket */
	need_sockname = true;
	printf("%d: socket ", fd);
	break;
    /* S_IFWHT is used as argument to mknod() to white-out
     * a name in a UnionFS directory, and doesn't appear
     * as the type of any actual inodes */
    default:
	printf("%d: unexpected stat mode %d\n", fd, sb.st_mode & S_IFMT);
	return;
    }

    if (need_path)
    {
	char path[PATH_MAX];
	memset(path, 0, sizeof(path));
	r = fcntl(fd, F_GETPATH, path);
	if (r == 0)
	{
	    printf("path \"%s\"\n", path);
	    return;
	}
	perror("F_GETPATH");
    }
    if (need_sockname)
    {
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);
	r = getsockname(fd, (struct sockaddr *)&addr, &len);
	if (r == 0)
	{
	    printf("name ");
	    describe_sockaddr((struct sockaddr *)&addr);
	    len = sizeof(addr);
	    r = getpeername(fd, (struct sockaddr *)&addr, &len);
	    if (r == 0)
	    {
		printf(" peer ");
		describe_sockaddr((struct sockaddr *)&addr);
	    }
	    printf("\n");
	}
    }
}

static void set_nonblock(int fd)
{
    int r;

    r = fcntl(fd, F_GETFL, /*ignored*/0);
    if (r < 0)
    {
	perror("fcntl(F_GETFL)");
	exit(1);
    }
    r = fcntl(fd, F_SETFL, r | O_NONBLOCK);
    if (r < 0)
    {
	perror("fcntl(F_SETFL)");
	exit(1);
    }
}

static void wait_for(int fd)
{
    fd_set rfd;
    int r;

    printf("waiting for fd %d\n", fd);
    FD_ZERO(&rfd);
    FD_SET(fd, &rfd);
    r = select(fd+1, &rfd, &rfd, &rfd, NULL);
}

int main(int argc, char **argv)
{
    int fd;
    int fd2;
    int fd3;
    DIR *dir;
    int r;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
    socklen_t len;
    static const char blockdev[] = "blockdev";
    static const char chardev[] = "chardev";
    int pfd[2];

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    static const char filename[] = "foo.dat";
    printf("Creating file %s\n", filename);
    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0)
    {
	perror(filename);
	exit(1);
    }
    printf("Expecting a regular file\n");
    describe_fd(fd);
    close(fd);
    printf("Expecting a bad file descriptor\n");
    describe_fd(fd);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    dir = opendir(".");
    printf("Expecting a directory\n");
    describe_fd(dirfd(dir));
    closedir(dir);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
	perror("socket");
	exit(1);
    }
    printf("Expecting an unbound IPV4 socket\n");
    describe_fd(fd);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_LOCALHOST;
    /* leave sin_port as zero, let the kernel choose a port, */
    r = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
    if (r < 0)
    {
	perror("bind");
	exit(1);
    }
    printf("Expecting a bound IPV4 socket\n");
    describe_fd(fd);
    r = listen(fd, 5);
    if (r < 0)
    {
	perror("listen");
	exit(1);
    }
    len = sizeof(sin);
    r = getsockname(fd, (struct sockaddr *)&sin, &len);
    if (r < 0)
    {
	perror("getsockname");
	exit(1);
    }

    fd2 = socket(PF_INET, SOCK_STREAM, 0);
    if (fd2 < 0)
    {
	perror("socket");
	exit(1);
    }
    set_nonblock(fd2);
    r = connect(fd2, (const struct sockaddr *)&sin, sizeof(sin));
    if (r == 0)
    {
	fprintf(stderr, "connect() unexpectedly succeeded\n");
	exit(1);
    }
    if (errno != EINPROGRESS)
    {
	perror("connect");
	exit(1);
    }
    fd3 = accept(fd, NULL, NULL);
    if (fd3 < 0)
    {
	perror("accept");
	exit(1);
    }
    wait_for(fd2);
    printf("Expecting a bound unconnected IPV4 socket\n");
    describe_fd(fd);
    printf("Expecting a connected IPV4 client socket\n");
    describe_fd(fd2);
    printf("Expecting a connected IPV4 server socket\n");
    describe_fd(fd3);
    close(fd);
    close(fd2);
    close(fd3);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    fd = socket(PF_INET6, SOCK_STREAM, 0);
    if (fd < 0)
    {
	perror("socket");
	exit(1);
    }
    printf("Expecting an unbound IPV6 socket\n");
    describe_fd(fd);

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = in6addr_loopback;
    /* leave sin6_port as zero, let the kernel choose a port, */
    r = bind(fd, (struct sockaddr *)&sin6, sizeof(sin6));
    if (r < 0)
    {
	perror("bind");
	exit(1);
    }
    printf("Expecting a bound IPV6 socket\n");
    describe_fd(fd);
    r = listen(fd, 5);
    if (r < 0)
    {
	perror("listen");
	exit(1);
    }
    len = sizeof(sin6);
    r = getsockname(fd, (struct sockaddr *)&sin6, &len);
    if (r < 0)
    {
	perror("getsockname");
	exit(1);
    }

    fd2 = socket(PF_INET6, SOCK_STREAM, 0);
    if (fd2 < 0)
    {
	perror("socket");
	exit(1);
    }
    set_nonblock(fd2);
    r = connect(fd2, (const struct sockaddr *)&sin6, sizeof(sin6));
    if (r == 0)
    {
	fprintf(stderr, "connect() unexpectedly succeeded\n");
	exit(1);
    }
    if (errno != EINPROGRESS)
    {
	perror("connect");
	exit(1);
    }
    fd3 = accept(fd, NULL, NULL);
    if (fd3 < 0)
    {
	perror("accept");
	exit(1);
    }
    wait_for(fd2);
    printf("Expecting a bound unconnected IPV6 socket\n");
    describe_fd(fd);
    printf("Expecting a connected IPV6 client socket\n");
    describe_fd(fd2);
    printf("Expecting a connected IPV6 server socket\n");
    describe_fd(fd3);
    close(fd);
    close(fd2);
    close(fd3);
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    fd = open("fifo", O_RDONLY|O_NONBLOCK, 0);
    printf("Expecting a named pipe\n");
    describe_fd(fd);
    close(fd);

#if 0
    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#if 0
    r = mknod(chardev, S_IFBLK|0644, makedev(4, 20));
    if (r < 0 && errno != EEXIST)
    {
	perror(chardev);
	exit(1);
    }
#endif
    fd = open(chardev, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(chardev);
	exit(1);
    }
    printf("Expecting a char device, major 4 minor 20\n");
    describe_fd(fd);
    close(fd);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#if 0
    r = mknod(blockdev, S_IFBLK|0644, makedev(4, 20));
    if (r < 0 && errno != EEXIST)
    {
	perror(blockdev);
	exit(1);
    }
#endif
    fd = open(blockdev, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(blockdev);
	exit(1);
    }
    printf("Expecting a block device, major 4 minor 20\n");
    describe_fd(fd);
    close(fd);
#endif

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    r = pipe(pfd);
    if (r < 0)
    {
	perror("pipe");
	exit(1);
    }
    printf("Expecting a pipe\n");
    describe_fd(pfd[0]);
    printf("Expecting another pipe\n");
    describe_fd(pfd[1]);
    close(pfd[0]);
    close(pfd[1]);

    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    return 0;
}
