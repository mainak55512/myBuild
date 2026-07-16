#include "container.h"
#include "yyjson.h"
#include <mybuild.h>

void create_my_build_config(char *config_file_path, char *project_name,
							char *project_lang, char *compiler_path,
							bool isExec) {
	Arena *str_arena = arena_init(1024);
	yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);

	yyjson_mut_obj_add_str(doc, root, "project_name", project_name);
	yyjson_mut_obj_add_str(doc, root, "project_language", project_lang);
	yyjson_mut_obj_add_str(doc, root, "compiler_path", compiler_path);
	yyjson_mut_obj_add_bool(doc, root, "executable", isExec);

	yyjson_mut_val *headers = yyjson_mut_arr(doc);
	yyjson_mut_val *sources = yyjson_mut_arr(doc);
	yyjson_mut_val *flags = yyjson_mut_arr(doc);
	yyjson_mut_val *lib_links = yyjson_mut_arr(doc);

	if (isExec) {
		yyjson_mut_arr_add_str(doc, sources, "src");
	} else {
		yyjson_mut_arr_add_str(doc, sources, "lib");
	}
	yyjson_mut_obj_add_val(doc, root, "include_paths", headers);
	yyjson_mut_obj_add_val(doc, root, "src", sources);
	yyjson_mut_obj_add_val(doc, root, "flags", flags);
	yyjson_mut_obj_add_val(doc, root, "lib_links", lib_links);

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

int generate_compile_commands() {
	Arena *str_arena = arena_init(1024);
	char *input_file = "myBuild.json";
	char *output_file = "compile_commands.json";
	int success = 1;

	yyjson_read_err err;
	yyjson_doc *doc = yyjson_read_file(input_file, 0, NULL, &err);

	if (!doc) {
		fprintf(stderr, "Failed to read %s: %s\n", input_file, err.msg);
		// return -1;
		success = 0;
		goto CLEANUP;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);

	yyjson_val *compiler_val = yyjson_obj_get(root, "compiler_path");
	const char *compiler = yyjson_get_str(compiler_val);
	String *cwd = get_current_working_dir(str_arena);

	Vector *include_paths = vector_init(char *);

	yyjson_val *inc_arr = yyjson_obj_get(root, "include_paths");
	if (yyjson_is_arr(inc_arr)) {
		yyjson_arr_iter iter;
		success = yyjson_arr_iter_init(inc_arr, &iter);
		if (!success) {
			fprintf(stderr, "Non an iterator: include_paths\n");
			success = 0;
			vector_free(include_paths);
			goto CLEANUP;
		}
		yyjson_val *val;
		while ((val = yyjson_arr_iter_next(&iter))) {
			append(char *, include_paths,
				   string(string_concat_cstr(str_arena, 3, string(cwd), "/",
											 (char *)yyjson_get_str(val))));
		}
	}

	yyjson_val *deps = yyjson_obj_get(root, "dependencies");
	if (yyjson_is_obj(deps)) {
		yyjson_obj_iter iter;
		success = yyjson_obj_iter_init(deps, &iter);
		if (!success) {
			fprintf(stderr, "Non an iterator: deps\n");
			success = 0;
			vector_free(include_paths);
			goto CLEANUP;
		}
		yyjson_val *key, *dep_obj;
		while ((key = yyjson_obj_iter_next(&iter))) {
			dep_obj = yyjson_obj_iter_get_val(key);
			const char *dep_name = yyjson_get_str(key);

			yyjson_val *dep_inc = yyjson_obj_get(dep_obj, "include_paths");
			if (yyjson_is_arr(dep_inc)) {
				yyjson_arr_iter inc_iter;
				success = yyjson_arr_iter_init(dep_inc, &inc_iter);
				if (!success) {
					fprintf(stderr, "Non an iterator: deps_include_paths\n");
					success = 0;
					vector_free(include_paths);
					goto CLEANUP;
				}
				yyjson_val *inc_val;
				while ((inc_val = yyjson_arr_iter_next(&inc_iter))) {
					String *path = string_concat_cstr(
						str_arena, 5, string(cwd), "/deps/", (char *)dep_name,
						"/", (char *)yyjson_get_str(inc_val));
					append(char *, include_paths, string(path));
				}
			}
		}
	}

	Vector *source_files = vector_init(char *);
	get_src_vec(str_arena, source_files, root, deps, cwd);

	yyjson_mut_doc *out_doc = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *out_root = yyjson_mut_arr(out_doc);
	yyjson_mut_doc_set_root(out_doc, out_root);

	String *includes = string_from(str_arena, "");
	for (size_t i = 0; i < length(include_paths); i++) {
		includes = string_concat_cstr(str_arena, 3, string(includes), " -I",
									  at(char *, include_paths, i));
	}

	for (size_t i = 0; i < length(source_files); i++) {
		yyjson_mut_val *entry = yyjson_mut_arr_add_obj(out_doc, out_root);

		success =
			yyjson_mut_obj_add_str(out_doc, entry, "directory", string(cwd));
		if (!success) {
			fprintf(stderr,
					"Error encountered while generating compile commands\n");
			success = 0;
			vector_free(include_paths);
			vector_free(source_files);
			yyjson_mut_doc_free(out_doc);
			goto CLEANUP;
		}
		success = yyjson_mut_obj_add_str(out_doc, entry, "file",
										 at(char *, source_files, i));
		if (!success) {
			fprintf(stderr,
					"Error encountered while generating compile commands\n");
			success = 0;
			vector_free(include_paths);
			vector_free(source_files);
			yyjson_mut_doc_free(out_doc);
			goto CLEANUP;
		}

		String *command = string_from(str_arena, "");
		command = string_concat_cstr(
			str_arena, 7, (char *)compiler, " -c ", at(char *, source_files, i),
			string(includes), " -o ", at(char *, source_files, i), ".o");

		success =
			yyjson_mut_obj_add_str(out_doc, entry, "command", string(command));
		if (!success) {
			fprintf(stderr,
					"Error encountered while generating compile commands\n");
			success = 0;
			vector_free(include_paths);
			vector_free(source_files);
			yyjson_mut_doc_free(out_doc);
			goto CLEANUP;
		}
	}

	yyjson_write_err write_err;
	yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
	success =
		yyjson_mut_write_file(output_file, out_doc, flg, NULL, &write_err);

	if (!success) {
		fprintf(stderr, "Failed to write %s: %s\n", output_file, write_err.msg);
	} else {
		printf("Generated %s with %d source files\n", output_file,
			   length(source_files));
	}

	vector_free(include_paths);
	vector_free(source_files);
	yyjson_mut_doc_free(out_doc);

CLEANUP:
	yyjson_doc_free(doc);
	arena_free(&str_arena);

	return success ? 0 : -1;
}

bool is_mybuild_config_present(char *filename) {
	FILE *file = fopen(filename, "r");
	if (file) {
		fclose(file);
		return true;
	}
	return false;
}

String *get_flags(Arena *str_arena, yyjson_val *root, String *type) {
	Arena *local_arena = arena_init(1024);
	String *flag_list = string_from(local_arena, "");

	yyjson_val *flag_arr;

	if (STR_CMP(string(type), "build") == 0) {
		flag_arr = yyjson_obj_get(root, "flags");
	} else if (STR_CMP(string(type), "lib") == 0) {
		flag_arr = yyjson_obj_get(root, "lib_links");
	}

	size_t idx = 0, max = 0;
	yyjson_val *val;

	yyjson_arr_foreach(flag_arr, idx, max, val) {
		if (string_len(flag_list) == 0) {
			flag_list = string_from(local_arena, (char *)yyjson_get_str(val));
		} else {
			flag_list = string_concat_cstr(local_arena, 3, string(flag_list),
										   "\n", (char *)yyjson_get_str(val));
		}
	}

	String *export_list = string_from(str_arena, string(flag_list));
	arena_free(&local_arena);
	return export_list;
}
