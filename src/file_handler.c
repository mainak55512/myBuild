#include <mybuild.h>

int create_append_file(char *file_path, char *content) {
	FILE *fp = fopen(file_path, "w+");
	if (fp == NULL) {
		perror("fopen failed");
		return 1;
	}
	fprintf(fp, "%s", content);
	fclose(fp);
	return 0;
}

String *collect_files(Arena *str_arena, String *path, String *type) {
	String *src_files = string_from(str_arena, "");
	String *ext_1 = string_from(str_arena, ".c");
	String *ext_2 = string_from(str_arena, ".cpp");

	char sep[2] = {'\n', '\0'};

	if (STR_CMP(string(type), "static") == 0) {
		sep[0] = ' ';
		ext_1 = string_from(str_arena, ".a");
		ext_2 = string_from(str_arena, ".lib");
	} else if (STR_CMP(string(type), "dyn") == 0) {
		sep[0] = ' ';
		ext_1 = string_from(str_arena, ".so");
		ext_2 = string_from(str_arena, ".dll");
	}

#ifdef _WIN32
	WIN32_FIND_DATA findData;
	HANDLE handleFind;
	String *searchPath;

	// Create search pattern (path\*.*)
	// snprintf(searchPath, MAX_PATH, "%s\\*.*", string(path));
	searchPath = string_concat_cstr(str_arena, 2, string(path), "\\*.*");

	handleFind = FindFirstFile(string(searchPath), &findData);

	if (handleFind == INVALID_HANDLE_VALUE) {
		printf("Unable to open directory: %s\n", path);
		return;
	}

	do {
		if (STR_CMP(findData.cFileName, ".") == 0 ||
			STR_CMP(findData.cFileName, "..") == 0) {
			continue;
		}
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		size_t len = strlen(findData.cFileName);

		if ((len > 2 &&
			 STR_CMP(findData.cFileName + len - 2, string(ext_1)) == 0) ||
			(len > 4 &&
			 STR_CMP(findData.cFileName + len - 4, string(ext_2)) == 0)) {

			if (strlen(string(src_files)) > 0) {
				src_files =
					string_concat_cstr(str_arena, 2, string(src_files), sep);
			}

			src_files =
				string_concat_cstr(str_arena, 4, string(src_files),
								   string(path), "\\", findData.cFileName);
		}
	} while (FindNextFile(handleFind, &findData) != 0);

	FindClose(handleFind);
#else
	DIR *dir;
	struct dirent *entry;

	dir = opendir(string(path));

	if (dir == NULL) {
		perror("Unable to open directory");
		return NULL;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (STR_CMP(entry->d_name, ".") == 0 ||
			STR_CMP(entry->d_name, "..") == 0) {
			continue;
		}
		char *dot = strrchr(entry->d_name, '.');
		if (dot != NULL && (STR_CMP(dot, string(ext_1)) == 0 ||
							STR_CMP(dot, string(ext_2)) == 0)) {

			if (strlen(string(src_files)) > 0) {
				src_files =
					string_concat_cstr(str_arena, 2, string(src_files), sep);
			}

			src_files = string_concat_cstr(str_arena, 4, string(src_files),
										   string(path), "/", entry->d_name);
		}
	}
	closedir(dir);
#endif
	return src_files;
}

void get_src_vec(Arena *str_arena, Vector *source_files, yyjson_val *root,
				 yyjson_val *deps, String *cwd) {

	yyjson_val *src_arr = yyjson_obj_get(root, "src");
	if (yyjson_is_arr(src_arr)) {
		yyjson_arr_iter iter;
		yyjson_arr_iter_init(src_arr, &iter);
		yyjson_val *val;
		while ((val = yyjson_arr_iter_next(&iter))) {
			Vector *src_temp_arr = string_split_lines(
				str_arena,
				collect_files(
					str_arena,
					string_from(str_arena, (char *)yyjson_get_str(val)),
					string_from(str_arena, "src")));
			for (int i = 0; i < length(src_temp_arr); i++) {
				char *elem = string(at(String *, src_temp_arr, i));
				if (STR_CMP(elem, "") != 0) {
					append(char *, source_files, elem);
				}
			}
			vector_free(src_temp_arr);
		}
	}

	if (yyjson_is_obj(deps)) {
		yyjson_obj_iter iter;
		yyjson_obj_iter_init(deps, &iter);
		yyjson_val *key, *dep_obj;
		while ((key = yyjson_obj_iter_next(&iter))) {
			dep_obj = yyjson_obj_iter_get_val(key);
			const char *dep_name = yyjson_get_str(key);

			yyjson_val *dep_src = yyjson_obj_get(dep_obj, "src");
			if (yyjson_is_arr(dep_src)) {
				yyjson_arr_iter src_iter;
				yyjson_arr_iter_init(dep_src, &src_iter);
				yyjson_val *src_val;
				while ((src_val = yyjson_arr_iter_next(&src_iter))) {
					String *path = string_concat_cstr(
						str_arena, 5, string(cwd), "/deps/", (char *)dep_name,
						"/", (char *)yyjson_get_str(src_val));
					Vector *src_temp_arr = string_split_lines(
						str_arena,
						collect_files(str_arena, path,
									  string_from(str_arena, "src")));
					for (int i = 0; i < length(src_temp_arr); i++) {
						char *elem = string(at(String *, src_temp_arr, i));
						if (STR_CMP(elem, "") != 0) {
							append(char *, source_files, elem);
						}
					}
					vector_free(src_temp_arr);
				}
			}
		}
	}
}

long long get_file_modified_time(const char *path) {
	struct stat attr;
	if (stat(path, &attr) == 0) {
		return (long long)attr.st_mtime;
	}
	return 0;
}

bool are_headers_newer(const char *d_file_path, long long obj_time) {
	FILE *f = fopen(d_file_path, "r");
	if (!f)
		return false;

	char token[1024];
	bool should_recompile = false;

	while (fscanf(f, "%1023s", token) == 1) {
		size_t len = strlen(token);
		if (len == 0)
			continue;

		if (strcmp(token, "\\") == 0)
			continue;

		if (token[len - 1] == ':') {
			token[len - 1] = '\0';
			len--;

			if (len == 0)
				continue;
		}

		if (strstr(token, ".cache/") != NULL && strstr(token, ".o") != NULL) {
			continue;
		}

		long long dep_file_time = get_file_modified_time(token);
		if (dep_file_time > obj_time) {
			should_recompile = true;
			break;
		}
	}

	fclose(f);
	return should_recompile;
}

bool directory_exists(const char *path) {
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	struct stat stats;
	return (stat(path, &stats) == 0 && S_ISDIR(stats.st_mode));
#endif
}
