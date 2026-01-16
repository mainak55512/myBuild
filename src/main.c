#include <mybuild.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int createOrAppendFile(char *file_path, char *content) {
	FILE *fp = fopen(file_path, "w+");
	if (fp == NULL) {
		perror("fopen failed");
		return 1;
	}
	fprintf(fp, "%s", content);
	fclose(fp);
	return 0;
}

void createMyBuildConfig(char *config_file_path, char *project_name,
						 char *project_lang, char *compiler_path) {
	Arena *str_arena = arena_init(200);
	yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);

	yyjson_mut_obj_add_str(doc, root, "project_name", project_name);
	yyjson_mut_obj_add_str(doc, root, "project_language", project_lang);
	yyjson_mut_obj_add_str(doc, root, "compiler_path", compiler_path);

	yyjson_mut_val *headers = yyjson_mut_arr(doc);
	yyjson_mut_val *sources = yyjson_mut_arr(doc);

	yyjson_mut_arr_add_str(doc, sources, "src");
	yyjson_mut_obj_add_val(doc, root, "include_paths", headers);
	yyjson_mut_obj_add_val(doc, root, "src", sources);

	yyjson_mut_arr_add_str(doc, headers, "include");
	// yyjson_mut_arr_add_str(doc, headers, "./deps/include");

	yyjson_mut_val *dependencies = yyjson_mut_obj(doc);
	yyjson_mut_obj_add_val(doc, root, "dependencies", dependencies);

	yyjson_write_err err;
	yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
	if (!yyjson_mut_write_file(config_file_path, doc, flg, NULL, &err)) {
		fprintf(stderr, "Write error: %s\n", err.msg);
		goto CLEANUP;
	}
CLEANUP:
	yyjson_mut_doc_free(doc);
	arena_free(&str_arena);
}

String *generateBuildList(Arena *str_arena, String *path) {
	String *src_files = string_from(str_arena, "");
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	char searchPath[MAX_PATH];

	// Create search pattern (path\*.*)
	snprintf(searchPath, MAX_PATH, "%s\\*.*", string(path));

	hFind = FindFirstFile(searchPath, &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Unable to open directory: %s\n", path);
		return;
	}

	do {
		// Skip "." and ".." entries
		if (STR_CMP(findData.cFileName, ".") == 0 ||
			STR_CMP(findData.cFileName, "..") == 0) {
			continue;
		}

		// Get the length of the filename
		size_t len = strlen(findData.cFileName);

		// Check if filename ends with .c or .cpp
		if ((len > 2 && strcmp(findData.cFileName + len - 2, ".c") == 0) ||
			(len > 4 && strcmp(findData.cFileName + len - 4, ".cpp") == 0)) {
			printf("%s\n", findData.cFileName);
		}
	} while (FindNextFile(hFind, &findData) != 0);

	FindClose(hFind);
#else
	DIR *dir;
	struct dirent *entry;

	dir = opendir(string(path));

	if (dir == NULL) {
		perror("Unable to open directory");
		return NULL;
	}

	// printf("Path: %s\n", string(path));

	while ((entry = readdir(dir)) != NULL) {
		if (STR_CMP(entry->d_name, ".") == 0 ||
			STR_CMP(entry->d_name, "..") == 0) {
			continue;
		}
		char *dot = strrchr(entry->d_name, '.');
		if (dot != NULL &&
			(STR_CMP(dot, ".c") == 0 || STR_CMP(dot, ".cpp") == 0)) {
			// printf("%s\n", entry->d_name);

			src_files =
				string_concat_cstr(str_arena, 6, string(src_files), "\n\"",
								   string(path), "/", entry->d_name, "\"");
		}
	}
	// printf("Src files: %s\n", string(src_files));
	closedir(dir);
#endif
	return src_files;
}

int checkProjectLang(char *lang) {
	if (STR_CMP(lang, "c++") == 0 || STR_CMP(lang, "cpp") == 0) {
		return 0;
	}
	return 1;
}

void generateCompileFlags() {
	Arena *str_arena = arena_init(1024);
	yyjson_read_err err;
	yyjson_doc *current_doc = yyjson_read_file("./myBuild.json", 0, NULL, &err);
	yyjson_val *current_root = yyjson_doc_get_root(current_doc);

	yyjson_val *headers = yyjson_obj_get(current_root, "include_paths");
	int idx = 0, max = 0;
	yyjson_val *val, *key;
	String *header_str = string_from(str_arena, "");
	yyjson_arr_foreach(headers, idx, max, val) {
		header_str = string_concat_cstr(str_arena, 4, string(header_str),
										"-I./", yyjson_get_str(val), "\n");
	}
	yyjson_val *dependencies = yyjson_obj_get(current_root, "dependencies");
	idx = 0, max = 0;
	yyjson_obj_foreach(dependencies, idx, max, key, val) {
		yyjson_val *dep_headers = yyjson_obj_get(val, "include_paths");
		int index = 0, max_val = 0;
		yyjson_val *elem;
		yyjson_arr_foreach(dep_headers, index, max_val, elem) {
			header_str = string_concat_cstr(
				str_arena, 7, string(header_str), "-I./", "deps/",
				yyjson_get_str(key), "/", yyjson_get_str(elem), "\n");
		}
	}
	createOrAppendFile("./compile_flags.txt", string(header_str));
	yyjson_doc_free(current_doc);
	arena_free(&str_arena);
}

int initProject() {
	struct STAT info;
	Arena *str_arena;
	String *project_name, *project_dir, *project_lang, *dep_dir, *dep_incl,
		*dep_lib, *compiler_path;
	int ret, dir_create_status, file_create_status;

	ret = 0;
	str_arena = arena_init(4096);
	printf("Project Name: ");
	project_name = string_trim(str_arena, string_get(str_arena));

	project_dir = string_from(str_arena, ".");

	printf("Language (c/c++): ");
	project_lang = string_trim(str_arena, string_get(str_arena));

	printf("Compiler path (default: gcc): ");
	compiler_path = string_trim(str_arena, string_get(str_arena));

	if (STR_CMP(string(compiler_path), "") == 0) {
		compiler_path = string_from(str_arena, "gcc");
	}

	dep_dir = string_concat_cstr(str_arena, 2, string(project_dir), "/deps");
	dir_create_status = MAKE_DIR(string(dep_dir));
	if (dir_create_status != 0) {
		ret = 1;
		goto CLEANUP;
	}

	dir_create_status = MAKE_DIR(
		string(string_concat_cstr(str_arena, 2, string(project_dir), "/src")));

	if (dir_create_status != 0) {
		ret = 1;
		goto CLEANUP;
	}
	dir_create_status = MAKE_DIR(string(
		string_concat_cstr(str_arena, 2, string(project_dir), "/include")));

	if (dir_create_status != 0) {
		ret = 1;
		goto CLEANUP;
	}

	switch (checkProjectLang(string(project_lang))) {
	case 0: {
		file_create_status = createOrAppendFile(
			string(string_concat_cstr(str_arena, 2, string(project_dir),
									  "/src/main.cpp")),
			"#include <iostream>\n\nint main() "
			"{\n\tstd::cout << \"Hello, World!\" << std::endl;\n}");

		if (file_create_status == 1) {
			ret = 1;
			goto CLEANUP;
		}
		file_create_status =
			createOrAppendFile(string(string_concat_cstr(
								   str_arena, 4, string(project_dir),
								   "/include/", string(project_name), ".hpp")),
							   "");

		if (file_create_status == 1) {
			ret = 1;
			goto CLEANUP;
		}
		createMyBuildConfig(
			string(string_concat_cstr(str_arena, 2, string(project_dir),
									  "/myBuild.json")),
			string(project_name), "cpp", string(compiler_path));
		break;
	}
	default: {
		file_create_status = createOrAppendFile(
			string(string_concat_cstr(str_arena, 2, string(project_dir),
									  "/src/main.c")),
			"#include <stdio.h>\n\nint main() "
			"{\n\tprintf(\"Hello, World!\");\n}");

		if (file_create_status == 1) {
			ret = 1;
			goto CLEANUP;
		}
		file_create_status = createOrAppendFile(
			string(string_concat_cstr(str_arena, 4, string(project_dir),
									  "/include/", string(project_name), ".h")),
			"");
		if (file_create_status == 1) {
			ret = 1;
			goto CLEANUP;
		}
		createMyBuildConfig(
			string(string_concat_cstr(str_arena, 2, string(project_dir),
									  "/myBuild.json")),
			string(project_name), "c", string(compiler_path));
		break;
	}
	}
	generateCompileFlags();

CLEANUP:
	arena_free(&str_arena);
	return ret;
}

String *buildProject(Arena *global_str_arena) {
	String *command;

	MAKE_DIR("build");

	Arena *str_arena = arena_init(1024);

	yyjson_read_err err;
	yyjson_doc *doc = yyjson_read_file("./myBuild.json", 0, NULL, &err);

	if (!doc) {
		fprintf(stderr, "Read error: %s\n", err.msg);
		goto CLEANUP;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	String *project_name = string_from(
		str_arena,
		(char *)yyjson_get_str(yyjson_obj_get(root, "project_name")));

	yyjson_val *header_arr = yyjson_obj_get(root, "include_paths");
	yyjson_val *src_arr = yyjson_obj_get(root, "src");
	yyjson_val *dep_arr = yyjson_obj_get(root, "dependencies");
	yyjson_val *compiler_path = yyjson_obj_get(root, "compiler_path");

	String *header_list = string_from(str_arena, "");
	String *src_list = string_from(str_arena, "");
	String *compiler = string_concat_cstr(
		str_arena, 2, (char *)yyjson_get_str(compiler_path), " ");
	size_t idx = 0, max = 0;
	yyjson_val *val, *key;

	yyjson_arr_foreach(header_arr, idx, max, val) {
		header_list =
			string_concat_cstr(str_arena, 4, string(header_list), "\n\"-I./",
							   (char *)yyjson_get_str(val), "\"");
	}

	idx = 0, max = 0;

	yyjson_arr_foreach(src_arr, idx, max, val) {
		src_list = string_concat_cstr(
			str_arena, 2, string(src_list),
			string(generateBuildList(
				str_arena, string_concat_cstr(str_arena, 2, "./",
											  (char *)yyjson_get_str(val)))));
	}

	idx = 0, max = 0;

	if (dep_arr) {
		yyjson_obj_foreach(dep_arr, idx, max, key, val) {
			yyjson_val *dep_src_arr, *dep_include_arr, *elem;
			int index, max_val;
			char *key_str = (char *)yyjson_get_str(key);

			// local = yyjson_obj_get(val, "local");
			dep_src_arr = yyjson_obj_get(val, "src");
			dep_include_arr = yyjson_obj_get(val, "include_paths");

			index = 0, max_val = 0;
			yyjson_arr_foreach(dep_src_arr, index, max_val, elem) {
				src_list = string_concat_cstr(
					str_arena, 2, string(src_list),
					string(generateBuildList(
						str_arena, string_concat_cstr(
									   str_arena, 4, "./deps/", key_str, "/",
									   (char *)yyjson_get_str(elem)))));
			}

			index = 0, max_val = 0;
			yyjson_arr_foreach(dep_include_arr, index, max_val, elem) {
				header_list = string_concat_cstr(
					str_arena, 7, string(header_list), "\n\"-I", "./deps/",
					key_str, "/", (char *)yyjson_get_str(elem), "\"");
			}
		}
	}

	yyjson_doc_free(doc);

	String *headers, *src;
	headers = string_trim(str_arena, header_list);
	src = string_trim(str_arena, src_list);

	// printf("Headers: %s\n", string(headers));
	// printf("SRCs: %s\n", string(src));

	String *response_content =
		string_concat_cstr(str_arena, 5, string(headers), string(src), "\n-o",
						   "\n./build/", string(project_name));

	MAKE_DIR("./build/.cache");
	createOrAppendFile("./build/.cache/build.rsp", string(response_content));

	/*
	command = string_concat_cstr(str_arena, 6, string(compiler), string(src),
								 string(headers), " -o ", "./build/",
								 string(project_name));

	// printf("Compile Command: %s\n", string(command));
	*/
	system(string(string_concat_cstr(str_arena, 2, string(compiler),
									 " @./build/.cache/build.rsp")));
	String *output = string_concat_cstr(global_str_arena, 2, "./build/",
										string(project_name));

CLEANUP:
	arena_free(&str_arena);
	return output;
}

char *get_repo_name(Arena *arena, const char *git_url) {
	if (!git_url)
		return NULL;

	// Find the last '/' in the URL
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

void addLibrary(char *libURL) {
	yyjson_read_err err;
	yyjson_doc *current_doc = yyjson_read_file("./myBuild.json", 0, NULL, &err);
	yyjson_val *current_root = yyjson_doc_get_root(current_doc);
	yyjson_val *dependencies = yyjson_obj_get(current_root, "dependencies");
	Vector *set = vector_init(char *);
	int idx = 0, max = 0;
	yyjson_val *val, *key;
	yyjson_obj_foreach(dependencies, idx, max, key, val) {
		yyjson_val *remote = yyjson_obj_get(val, "remote");
		set_add(set, (char *)yyjson_get_str(remote));
	}
	yyjson_doc_free(current_doc);
	if (!set_contains(set, libURL)) {
		set_add(set, libURL);
		fetchLibrary(set, libURL);
	}
	vector_free(set);
}

void fetchLibrary(Vector *v, char *libURL) {
	String *command, *repo_name, *dep_mybuild_path;
	Arena *str_arena;
	yyjson_read_err err;

	str_arena = arena_init(1024);
	repo_name = string_from(str_arena, get_repo_name(str_arena, libURL));
	command = string_concat_cstr(str_arena, 4, "git clone ", libURL, " ./deps/",
								 string(repo_name));
	system(string(command));

	yyjson_doc *current_doc = yyjson_read_file("./myBuild.json", 0, NULL, &err);
	yyjson_mut_doc *current_mut_doc = yyjson_doc_mut_copy(current_doc, NULL);

	yyjson_doc_free(current_doc);

	yyjson_mut_val *current_root = yyjson_mut_doc_get_root(current_mut_doc);
	yyjson_mut_val *dependencies =
		yyjson_mut_obj_get(current_root, "dependencies");

	dep_mybuild_path = string_concat_cstr(str_arena, 3, "./deps/",
										  string(repo_name), "/myBuild.json");

	// printf("Dep build path: %s\n", string(dep_mybuild_path));
	yyjson_doc *dep_doc =
		yyjson_read_file(string(dep_mybuild_path), 0, NULL, &err);

	yyjson_val *dep_root = yyjson_doc_get_root(dep_doc);

	yyjson_val *src = yyjson_obj_get(dep_root, "src");
	yyjson_val *headers = yyjson_obj_get(dep_root, "include_paths");
	char *version = (char *)yyjson_get_str(yyjson_obj_get(dep_root, "version"));

	yyjson_mut_val *dep_header_arr =
		yyjson_val_mut_copy(current_mut_doc, headers);
	yyjson_mut_val *dep_src_arr = yyjson_val_mut_copy(current_mut_doc, src);

	yyjson_mut_val *target_obj = yyjson_mut_obj(current_mut_doc);

	yyjson_mut_obj_add_str(current_mut_doc, target_obj, "version", version);
	yyjson_mut_obj_add(target_obj,
					   yyjson_mut_str(current_mut_doc, "include_paths"),
					   dep_header_arr);
	yyjson_mut_obj_add(target_obj, yyjson_mut_str(current_mut_doc, "src"),
					   dep_src_arr);

	yyjson_mut_obj_add_str(current_mut_doc, target_obj, "remote", libURL);
	yyjson_mut_obj_add(dependencies,
					   yyjson_mut_str(current_mut_doc, string(repo_name)),
					   target_obj);

	yyjson_write_err werr;
	yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
	if (!yyjson_mut_write_file("./myBuild.json", current_mut_doc, flg, NULL,
							   &werr)) {
		fprintf(stderr, "Write error: %s\n", err.msg);
	}

	generateCompileFlags();

	yyjson_mut_doc_free(current_mut_doc);

	yyjson_val *dep_dependencies = yyjson_obj_get(dep_root, "dependencies");
	int idx = 0, max = 0;
	yyjson_val *val, *key;
	yyjson_obj_foreach(dep_dependencies, idx, max, key, val) {
		yyjson_val *remote = yyjson_obj_get(val, "remote");
		if (!set_contains(v, (char *)yyjson_get_str(remote))) {
			set_add(v, (char *)yyjson_get_str(remote));
			fetchLibrary(v, (char *)yyjson_get_str(remote));
		}
	}
	yyjson_doc_free(dep_doc);
	arena_free(&str_arena);
	return;
}

void runProject(Arena *global_str_arena) {
	system(string(buildProject(global_str_arena)));
}

int main(int argc, char *argv[]) {
	Arena *global_str_arena = arena_init(1024);
	if (argc < 2) {
		printf("Usage: myBuild <command> [args]\n");
		printf("Commands: init, add <url>, build\n");
		return 1;
	}

	char *opt = argv[1];

	if (STR_CMP(opt, "init") == 0) {
		initProject();
	} else if (STR_CMP(opt, "add") == 0) {
		addLibrary(argv[2]);
	} else if (STR_CMP(opt, "build") == 0) {
		buildProject(global_str_arena);
	} else if (STR_CMP(opt, "run") == 0) {
		runProject(global_str_arena);
	} else {
		printf("Unknown command: %s\n", opt);
		return 1;
	}
	arena_free(&global_str_arena);
	return 0;
}
