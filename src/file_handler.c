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

String *collect_src_files(Arena *str_arena, String *path) {
	String *src_files = string_from(str_arena, "");
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	HANDLE handleFind;
	String *searchPath;

	// Create search pattern (path\*.*)
	snprintf(searchPath, MAX_PATH, "%s\\*.*", string(path));
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

		if ((len > 2 && STR_CMP(findData.cFileName + len - 2, ".c") == 0) ||
			(len > 4 && STR_CMP(findData.cFileName + len - 4, ".cpp") == 0)) {

			if (strlen(string(src_files)) > 0) {
				src_files =
					string_concat_cstr(str_arena, 2, string(src_files), "\n");
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
		if (dot != NULL &&
			(STR_CMP(dot, ".c") == 0 || STR_CMP(dot, ".cpp") == 0)) {

			if (strlen(string(src_files)) > 0) {
				src_files =
					string_concat_cstr(str_arena, 2, string(src_files), "\n");
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
				collect_src_files(
					str_arena,
					string_from(str_arena, (char *)yyjson_get_str(val))));
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
						str_arena, collect_src_files(str_arena, path));
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
