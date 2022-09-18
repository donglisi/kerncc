#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <utils.h>

extern char cc[];

bool check_is_cc(int argc, char **argv);
int native_cc(int argc, char **argv);

bool need_remote_cc(int argc, char **argv)
{
	if (check_is_cc(argc, argv)) {
		if (get_file_size(argv[argc - 1]) > 500) {
			srand(time(NULL) + getpid());
			if (rand() % 100 > 50)
				return true;
		}
	}

	return false;
}

int get_sockfd()
{
	int sockfd;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error : Could not create socket\n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);

	if (inet_pton(AF_INET, "192.168.1.2", &serv_addr.sin_addr) <= 0) {
		printf("inet_pton error occured\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error : Connect Failed\n");
		return -1;
	}

	return sockfd;
}

int remote_cc(int argc, char **argv)
{
	int sockfd, i, fd, n, len, es, ret = 0;
	char buf[BUFSIZ], *opath, *dpath, *cmd, **args;

	sockfd = get_sockfd();
	if (sockfd == -1)
		return native_cc(argc, argv);

	cmd = get_cmd(argc, argv);
	args = argc_argv_to_args(argc, argv);

	write_from_str(sockfd, cmd);

	read(sockfd, &es, sizeof(int));
	if (es < 0) {
		read_to_fd(sockfd, STDERR_FILENO);
		ret = -1;
		goto compile_error;
	}

	get_opath(args, &opath);
	fd = open(opath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);

	get_dpath(args, &dpath);
	fd = open(dpath, O_CREAT | O_WRONLY, 0644);
	read_to_fd(sockfd, fd);
	close(fd);
	free(dpath);

compile_error:
	free(cmd);
	close(sockfd);

	return ret;
}

int native_cc(int argc, char **argv)
{
	char **args;

	args = argc_argv_to_args(argc, argv);
	args[0] = cc;

	return execvp(args[0], args);
}

int main(int argc, char *argv[])
{
	int ret;

	if (need_remote_cc(argc, argv))
		ret = remote_cc(argc, argv);
	else
		ret = native_cc(argc, argv);
	return ret;
}

/*
 * compile native only
 */
static char *paths[] = {
			"/arch/x86/boot",
			"/arch/x86/entry",
			"/drivers/scsi/",
			"/drivers/gpu/drm/radeon/",
			"/crypto",
			"/security/selinux",
			"/security/keys/trusted-keys/trusted_tpm2.c",
			"/kernel/configs.c",
			"/arch/x86/lib/inat.c",
			"/lib/oid_registry.c",
			"/lib/crc",
			"/init/version.c",
			"/lib/zstd/common",
			NULL};

bool check_is_cc(int argc, char **argv)
{
	int i;
	char *cpath = argv[argc - 1];

	if (!end_with(cpath, ".c"))
		return false;

	if (cpath[0] != '/')
		return false;

	for (i = 0; paths[i]; i++)
		if (strstr(cpath, paths[i]))
			return false;

	return true;
}
