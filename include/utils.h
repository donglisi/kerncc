char *read_to_str(int fd);
char **get_args(char *cmd);
char **argc_argv_to_args(int argc, char **argv);

int get_file_size(char *path);
int end_with(const char *str, const char *suffix);
int write_to_fd(int infd, int outfd);

void write_from_str(int fd, char *str);
void read_to_fd(int infd, int outfd);
void print_argv(int argc, char **argv);
void print_args(char **args);
void dirname1(char *path, char **dir);
void basename1(char *path, char **name);
void get_opath(char **args, char **opath);
void get_dpath(char **args, char **dpath);
void mkdir_recursion(char *path);
