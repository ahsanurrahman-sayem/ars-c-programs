#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: mgcc <gcc arguments>\n");
		return 1;
	}

	// Detect output file
	char *output = "a.out";
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
			output = argv[i + 1];
		else if (strncmp(argv[i], "-o", 2) == 0 && strlen(argv[i]) > 2)
			output = argv[i] + 2;
	}

	// Spawn gcc process
	pid_t pid;
	int status;
	argv[0] = "gcc";
	if (posix_spawnp(&pid, "gcc", NULL, NULL, argv, environ) != 0)
	{
		perror("posix_spawnp gcc");
		return 1;
	}

	if (waitpid(pid, &status, 0) < 0)
	{
		perror("waitpid");
		return 1;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		fprintf(stderr, "mgcc: compilation failed\n");
		return 1;
	}

	// Spawn compiled program
	char run_path[512];
	snprintf(run_path, sizeof(run_path), "./%s", output);
	char *run_argv[] = { run_path, NULL };

	if (posix_spawnp(&pid, run_path, NULL, NULL, run_argv, environ) != 0)
	{
		perror("posix_spawnp run");
		return 1;
	}

	if (waitpid(pid, NULL, 0) < 0)
	{
		perror("waitpid");
		return 1;
	}

	return 0;
}