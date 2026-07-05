#include <mybuild.h>

const char *get_filename_without_path(const char *path) {
	const char *last_slash = strrchr(path, '/');
	const char *last_backslash = strrchr(path, '\\');

	const char *filename = path;
	if (last_slash && last_slash > filename) {
		filename = last_slash + 1;
	}
	if (last_backslash && last_backslash > filename) {
		filename = last_backslash + 1;
	}

	return filename;
}

Vector *string_split_lines(Arena *arena, String *str) {
	Vector *lines = vector_init(String *);
	char *cstr = string(str);
	int len = string_len(str);
	int start = 0;

	for (int i = 0; i < len; i++) {
		if (cstr[i] == '\n') {
			String *line = string_sub(arena, str, start, i);
			append(String *, lines, line);
			start = i + 1;
		}
	}

	if (start <= len) {
		String *line = string_sub(arena, str, start, len);
		append(String *, lines, line);
	}

	return lines;
}

int check_project_lang(char *lang) {
	if (STR_CMP(lang, "c++") == 0 || STR_CMP(lang, "cpp") == 0) {
		return 0;
	}
	return 1;
}

String *get_current_working_dir(Arena *arena) {
	char cwd[1024];
	GET_WD(cwd, sizeof(cwd));
	return string_from(arena, cwd);
}

char *get_repo_name(Arena *arena, const char *git_url) {
	if (!git_url)
		return NULL;

	const char *last_slash = strrchr(git_url, '/');
	if (!last_slash)
		return NULL;

	const char *repo_start = last_slash + 1;

	const char *git_suffix = strstr(repo_start, ".git");

	size_t len;
	if (git_suffix) {
		len = git_suffix - repo_start;
	} else {
		len = strlen(repo_start);
	}

	char *repo_name = (char *)arena_alloc(arena, len + 1);
	if (!repo_name)
		return NULL;

	strncpy(repo_name, repo_start, len);
	repo_name[len] = '\0';

	return repo_name;
}

bool set_contains(Vector *v, char *elem) {

	for (int i = 0; i < length(v); i++) {
		if (STR_CMP(at(char *, v, i), elem) == 0) {
			return true;
		}
	}
	return false;
}

void set_add(Vector *v, char *elem) {
	if (!set_contains(v, elem)) {
		append(char *, v, elem);
	}
}
