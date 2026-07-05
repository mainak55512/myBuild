#include <arena.h>
#include <container.h>
#include <cstring.h>
#include <sys/stat.h>
#include <yyjson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef _WIN32
#include <direct.h>
#include <string.h>
#include <windows.h>
#define MAKE_DIR(dir) _mkdir(dir)
#define STAT _stat
#define STR_CMP _stricmp
#define GET_WD _getcwd
#else
#include <dirent.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#define MAKE_DIR(dir) mkdir(dir, 0777)
#define STAT stat
#define STR_CMP strcasecmp
#define GET_WD getcwd
#endif

int create_append_file(char *file_path, char *content);
void create_my_build_config(char *config_file_path, char *project_name,
							char *project_lang, char *compiler_path,
							bool isExec);
int check_project_lang(char *lang);
String *build_project(Arena *global_str_arena);
void fetch_library(Vector *v, char *libURL, bool sync);
bool set_contains(Vector *v, char *elem);
void set_add(Vector *v, char *elem);
String *clone_lib(Arena *arena, char *libURL);
bool is_mybuild_config_present(char *filename);
int init_project();
String *collect_src_files(Arena *str_arena, String *path);
Vector *string_split_lines(Arena *arena, String *str);
String *get_current_working_dir(Arena *arena);
int generate_compile_commands();
String *build_project(Arena *global_str_arena);
char *get_repo_name(Arena *arena, const char *git_url);
void add_library(char *libURL);
void run_project(Arena *global_str_arena);
void sync_dependency();
void get_src_vec(Arena *str_arena, Vector *source_files, yyjson_val *root,
				 yyjson_val *deps, String *cwd);
const char *get_filename_without_path(const char *path);

int cli(int argc, char *argv[], Arena *global_str_arena);
