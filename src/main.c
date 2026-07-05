#include <mybuild.h>

int main(int argc, char *argv[]) {
	Arena *global_str_arena = arena_init(1024);

	int err = cli(argc, argv, global_str_arena);

	arena_free(&global_str_arena);
	return err;
}
