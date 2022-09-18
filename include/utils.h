char *get_cmd(int argc, char **argv);
char **get_args(char *cmd);
void get_opath(char **args, char **opath);
void get_dpath(char **args, char **dpath);
void get_epath(char **args, char **epath);
int get_file_size(char *path);
int end_with(const char *str, const char *suffix);
char *read_to_str(int fd);
void write_from_str(int fd, char *str);
void read_to_fd(int infd, int outfd);
void print_args(char **args);
void mkdir_recursion(char *path);
void dirname1(char *path, char **dir);
char *basename1(char *path, char **name);
char **argc_argv_to_args(int argc, char **argv);
