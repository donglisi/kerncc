#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>

void get_opath(int argc, char **argv, char **opath)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp("-o", argv[i])) {
			*opath = argv[i + 1];
			return;
		}
	}
}

void get_dpath(int argc, char **argv, char **dpath)
{
	int loc;
	char *opath, *name;

	get_opath(argc, argv, &opath);
	*dpath = malloc(strlen(opath) + 4);
	dpath[strlen(opath) + 3] = 0;

	strcpy(*dpath, opath);
	name = strrchr(opath, '/');
	loc = strlen(opath) - strlen(name);
	strcpy(&((*dpath)[loc + 1]), name);
	(*dpath)[loc] = '/';
	(*dpath)[loc + 1] = '.';
	strcpy(&((*dpath)[strlen(opath) + 1]), ".d");
}

int get_file_size(char *path)
{
	int fd;
	struct stat statbuf;

	fd = open(path, O_RDONLY);
	fstat(fd, &statbuf);
	close(fd);

	return statbuf.st_size;
}

