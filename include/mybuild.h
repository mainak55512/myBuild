#include <arena.h>
#include <container.h>
#include <cstring.h>
#include <sys/stat.h>
#include <yyjson.h>

#ifdef _WIN32
#include <direct.h>
#include <string.h>
#include <windows.h>
#define MAKE_DIR(dir) _mkdir(dir)
#define STAT _stat
#define STR_CMP _stricmp
#else
#include <dirent.h>
#include <strings.h>
#include <sys/types.h>
#define MAKE_DIR(dir) mkdir(dir, 0777)
#define STAT stat
#define STR_CMP strcasecmp
#endif

int createOrAppendFile(char *file_path, char *content);
void createMyBuildConfig(char *config_file_path, char *project_name,
						 char *project_lang, char *compiler_path);
int checkProjectLang(char *lang);
String *buildProject(Arena *global_str_arena);
void fetchLibrary(Vector *v, char *libURL);
int initProject();
