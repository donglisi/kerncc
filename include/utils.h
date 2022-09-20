char *read_to_str(int fd);
int write_from_str(int fd, char *str);

char **get_args(char *cmd);
char **argc_argv_to_args(int argc, char **argv);
void print_argv(int argc, char **argv);
void print_args(char **args);

int get_file_size(char *path);
void get_opath(char **args, char **opath);
void get_dpath(char **args, char **dpath);
void dirname1(char *path, char **dir);
void basename1(char *path, char **name);
void mkdir_recursion(char *path);

int end_with(const char *str, const char *suffix);
