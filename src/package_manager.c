#include <mybuild.h>

void update_package_file(yyjson_mut_doc *package) {
	yyjson_write_err werr;
	yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
	if (!yyjson_mut_write_file("./deps/.package", package, flg, NULL, &werr)) {
		fprintf(stderr, "Write error: %s\n", werr.msg);
	}
}

void sync_dependency() {

	char *myBuildConfigFile = "myBuild.json";
	char *packageFile = "deps/.package";
	yyjson_read_err err;
	yyjson_doc *buildConf = yyjson_read_file(myBuildConfigFile, 0, NULL, &err);
	if (!buildConf) {
		fprintf(stderr, "Failed to read %s: %s\n", myBuildConfigFile, err.msg);
		return;
	}
	yyjson_doc *packageConf = yyjson_read_file(packageFile, 0, NULL, &err);
	if (!packageConf) {
		fprintf(stderr, "Failed to read %s: %s\n", packageFile, err.msg);
		return;
	}

	yyjson_mut_doc *buildConf_mut = yyjson_doc_mut_copy(buildConf, NULL);
	yyjson_mut_doc *packageConf_mut = yyjson_doc_mut_copy(packageConf, NULL);

	yyjson_doc_free(buildConf);
	yyjson_doc_free(packageConf);

	yyjson_mut_val *build_root = yyjson_mut_doc_get_root(buildConf_mut);
	yyjson_mut_val *package_root = yyjson_mut_doc_get_root(packageConf_mut);

	yyjson_mut_val *deps = yyjson_mut_obj_get(build_root, "dependencies");
	yyjson_mut_val *inst_pkg = yyjson_mut_obj_get(package_root, "packages");

	Vector *installed = vector_init(char *);
	Vector *dep_arr = vector_init(char *);
	Vector *not_installed = vector_init(char *);

	if (yyjson_mut_is_arr(inst_pkg)) {
		yyjson_mut_arr_iter iter;
		yyjson_mut_arr_iter_init(inst_pkg, &iter);
		yyjson_mut_val *val;
		while ((val = yyjson_mut_arr_iter_next(&iter))) {
			set_add(installed, (char *)yyjson_mut_get_str(val));
		}
	}

	if (yyjson_mut_is_obj(deps)) {
		// Arena *local_arena = arena_init(1024);
		yyjson_mut_obj_iter iter;
		yyjson_mut_obj_iter_init(deps, &iter);
		yyjson_mut_val *key, *dep_obj;
		while ((key = yyjson_mut_obj_iter_next(&iter))) {
			dep_obj = yyjson_mut_obj_iter_get_val(key);
			const char *dep_name = yyjson_mut_get_str(key);

			yyjson_mut_val *dep_remote = yyjson_mut_obj_get(dep_obj, "remote");
			yyjson_mut_val *flags = yyjson_mut_obj_get(dep_obj, "flags");
			yyjson_mut_val *lib_links =
				yyjson_mut_obj_get(dep_obj, "lib_links");
			if (!set_contains(installed,
							  (char *)yyjson_mut_get_str(dep_remote))) {

				set_add(installed, (char *)yyjson_mut_get_str(dep_remote));

				// fetch_library(installed, (char
				// *)yyjson_mut_get_str(dep_remote), 			  true);

				fetch_library(installed, (char *)yyjson_mut_get_str(dep_remote),
							  flags, lib_links, true);
			}
		}

		yyjson_mut_val *package_arr = yyjson_mut_arr(packageConf_mut);

		for (int i = 0; i < length(installed); i++) {
			yyjson_mut_val *val =
				yyjson_mut_str(packageConf_mut, at(char *, installed, i));
			yyjson_mut_arr_append(package_arr, val);
		}
		yyjson_mut_obj_put(package_root,
						   yyjson_mut_str(packageConf_mut, "packages"),
						   package_arr);
		update_package_file(packageConf_mut);
		// arena_free(&local_arena);
	}

	generate_compile_commands();
	yyjson_mut_doc_free(buildConf_mut);
	yyjson_mut_doc_free(packageConf_mut);
	vector_free(installed);
	vector_free(dep_arr);
}

void add_library(char *libURL) {
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
		fetch_library(set, libURL, NULL, NULL, false);
	}
	yyjson_doc *package = yyjson_read_file("./deps/.package", 0, NULL, &err);
	yyjson_mut_doc *package_mut = yyjson_doc_mut_copy(package, NULL);
	yyjson_doc_free(package);
	yyjson_mut_val *root = yyjson_mut_doc_get_root(package_mut);
	yyjson_mut_val *package_arr = yyjson_mut_arr(package_mut);
	// yyjson_mut_val *package_arr = yyjson_mut_obj_get(root, "packages");

	for (int i = 0; i < length(set); i++) {
		yyjson_mut_val *val = yyjson_mut_str(package_mut, at(char *, set, i));
		yyjson_mut_arr_append(package_arr, val);
	}
	yyjson_mut_obj_put(root, yyjson_mut_str(package_mut, "packages"),
					   package_arr);
	// yyjson_mut_obj_add_val(package_mut, root, "packages", package_arr);

	generate_compile_commands();
	update_package_file(package_mut);
	yyjson_mut_doc_free(package_mut);
	vector_free(set);
}

String *clone_lib(Arena *arena, char *libURL) {
	String *repo_name = string_from(arena, get_repo_name(arena, libURL));
	printf("Installing %s...\n", string(repo_name));
	String *command = string_concat_cstr(arena, 4, "git clone --quiet ", libURL,
										 " ./deps/", string(repo_name));
	system(string(command));
	printf("Done!\n");
	return repo_name;
}

void fetch_library(Vector *v, char *libURL, yyjson_mut_val *sync_flags,
				   yyjson_mut_val *sync_lib_links, bool sync) {
	String *command, *repo_name, *dep_mybuild_path;
	Arena *str_arena;
	yyjson_read_err err;

	str_arena = arena_init(1024);
	repo_name = clone_lib(str_arena, libURL);

	String *config_path = string_concat_cstr(
		str_arena, 3, "./deps/", string(repo_name), "/myBuild.json");
	if (!is_mybuild_config_present(string(config_path))) {
		arena_free(&str_arena);
		return;
	}
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
	yyjson_val *dep_flags = yyjson_obj_get(dep_root, "flags");
	yyjson_val *dep_lib_links = yyjson_obj_get(dep_root, "lib_links");
	yyjson_mut_val *current_flags = yyjson_mut_obj_get(current_root, "flags");
	yyjson_mut_val *current_lib_links =
		yyjson_mut_obj_get(current_root, "lib_links");
	char *version = (char *)yyjson_get_str(yyjson_obj_get(dep_root, "version"));

	Vector *flag_vec = vector_init(char *);
	Vector *lib_link_vec = vector_init(char *);

	int idx = 0, max = 0;
	yyjson_val *val, *key;
	yyjson_mut_val *val_mut, *key_mut;

	yyjson_mut_arr_foreach(current_flags, idx, max, val_mut) {
		set_add(flag_vec, (char *)yyjson_mut_get_str(val_mut));
	}
	yyjson_arr_foreach(dep_flags, idx, max, val) {
		set_add(flag_vec, (char *)yyjson_get_str(val));
	}

	if (sync && yyjson_mut_is_arr(sync_flags)) {
		yyjson_mut_arr_foreach(sync_flags, idx, max, val_mut) {
			set_add(flag_vec, (char *)yyjson_mut_get_str(val_mut));
		}
	}

	idx = 0, max = 0;
	yyjson_mut_arr_foreach(current_lib_links, idx, max, val_mut) {
		set_add(lib_link_vec, (char *)yyjson_mut_get_str(val_mut));
	}
	yyjson_arr_foreach(dep_lib_links, idx, max, val) {
		set_add(lib_link_vec, (char *)yyjson_get_str(val));
	}
	if (sync && yyjson_mut_is_arr(sync_lib_links)) {
		yyjson_mut_arr_foreach(sync_lib_links, idx, max, val_mut) {
			set_add(lib_link_vec, (char *)yyjson_mut_get_str(val_mut));
		}
	}

	// yyjson_mut_obj_remove_str(target_obj, "flags");
	// yyjson_mut_obj_remove_str(target_obj, "lib_links");

	yyjson_mut_val *dep_header_arr =
		yyjson_val_mut_copy(current_mut_doc, headers);
	yyjson_mut_val *dep_src_arr = yyjson_val_mut_copy(current_mut_doc, src);

	if (current_flags != NULL) {
		yyjson_mut_arr_clear(current_flags);
		for (int i = 0; i < length(flag_vec); i++) {
			yyjson_mut_arr_add_str(current_mut_doc, current_flags,
								   at(char *, flag_vec, i));
		}
	} else {
		yyjson_mut_val *temp_flag_arr = yyjson_mut_arr(current_mut_doc);
		for (int i = 0; i < length(flag_vec); i++) {
			yyjson_mut_arr_add_str(current_mut_doc, temp_flag_arr,
								   at(char *, flag_vec, i));
		}
		yyjson_mut_obj_add_val(current_mut_doc, current_root, "flags",
							   temp_flag_arr);
	}
	if (current_lib_links != NULL) {
		yyjson_mut_arr_clear(current_lib_links);
		for (int i = 0; i < length(lib_link_vec); i++) {
			yyjson_mut_arr_add_str(current_mut_doc, current_lib_links,
								   at(char *, lib_link_vec, i));
		}
	} else {
		yyjson_mut_val *temp_lib_link_arr = yyjson_mut_arr(current_mut_doc);
		for (int i = 0; i < length(lib_link_vec); i++) {
			yyjson_mut_arr_add_str(current_mut_doc, temp_lib_link_arr,
								   at(char *, lib_link_vec, i));
		}
		yyjson_mut_obj_add_val(current_mut_doc, current_root, "lib_links",
							   temp_lib_link_arr);
	}

	if (!sync && !yyjson_mut_obj_get(dependencies, string(repo_name))) {
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
	}

	if (dependencies != NULL) {
		int d_idx = 0, d_max = 0;
		yyjson_mut_val *d_key, *d_val;
		yyjson_mut_obj_foreach(dependencies, d_idx, d_max, d_key, d_val) {
			if (yyjson_mut_is_obj(d_val)) {
				yyjson_mut_obj_remove_str(d_val, "flags");
				yyjson_mut_obj_remove_str(d_val, "lib_links");
			}
		}
	}

	yyjson_write_err werr;
	yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
	if (!yyjson_mut_write_file("./myBuild.json", current_mut_doc, flg, NULL,
							   &werr)) {
		fprintf(stderr, "Write error: %s\n", werr.msg);
	}

	yyjson_mut_doc_free(current_mut_doc);

	yyjson_val *dep_dependencies = yyjson_obj_get(dep_root, "dependencies");
	idx = 0, max = 0;
	yyjson_obj_foreach(dep_dependencies, idx, max, key, val) {
		yyjson_val *remote = yyjson_obj_get(val, "remote");
		if (!set_contains(v, (char *)yyjson_get_str(remote))) {
			set_add(v, (char *)yyjson_get_str(remote));
			fetch_library(v, (char *)yyjson_get_str(remote), NULL, NULL, false);
		}
	}
	vector_free(flag_vec);
	vector_free(lib_link_vec);
	yyjson_doc_free(dep_doc);
	arena_free(&str_arena);
	return;
}
