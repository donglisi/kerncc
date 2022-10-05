char *read_to_str(int fd);
int write_from_str(int fd, char *str);

char **argc_argv_to_args(int argc, char **argv);
char **get_args(char *cmd);
void free_args(char **args);
int get_argc(char **args);

void print_argv(int argc, char **argv);
void print_args(char **args);
void print_cpath(char **args);

int get_file_size(char *path);
int end_with(const char *str, const char *suffix);

int read_file_from_sockfd(int sockfd, char *path);
int write_file_to_sockfd(int connfd, char *path);

char *get_ipath(void);
char *get_opath(char *ipath);
char *get_izpath(char *ipath);

int compression(char *path, char *zpath);
int decompression(char *zpath, char *path);
