#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <xutils/file_map.h>
#include <xutils/simple_sock.h>

void arbitrary_demo(int firstSelB);

static int
usage(char *path, int rc)
{
	char *app = basename(path);
	printf("Usage: %s AB\n", app);
	printf("       %s BA\n", app);
	return (rc);
}

int
main(int argc, char *argv[])
{
	if (argc == 2) {
		if (!strcmp(argv[1], "AB")) {
			arbitrary_demo(0);
			return (0);
		} else if (!strcmp(argv[1], "BA")) {
			arbitrary_demo(1);
			return (0);
		} else
			return (usage(argv[0], -1));
	} else if (argc == 1)
		return (usage(argv[0], 0));
	else
		return (usage(argv[0], -1));
}
