#include <mybuild.h>

int cli(int argc, char *argv[], Arena *global_str_arena) {
	if (argc < 2) {
		printf("Usage: myBuild <command> [args]\n");
		printf("Commands: init, add <url>, build\n");
		return 1;
	}
	char *opt = argv[1];
	if (STR_CMP(opt, "init") == 0) {
		init_project();
		return 0;
	} else if (STR_CMP(opt, "add") == 0) {
		add_library(argv[2]);
		return 0;
	} else if (STR_CMP(opt, "build") == 0) {
		build_project(global_str_arena);
		return 0;
	} else if (STR_CMP(opt, "run") == 0) {
		run_project(global_str_arena);
		return 0;
	} else if (STR_CMP(opt, "gen") == 0) {
		generate_compile_commands();
		return 0;
	} else if (STR_CMP(opt, "sync") == 0) {
		sync_dependency();
		return 0;
	} else {
		printf("Unknown command: %s\n", opt);
		return 1;
	}
}
