#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <getopt.h>
#include <pthread.h>

static const char *setpid_version_string = "0.0.16";

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{"command", required_argument, NULL, 'C'},
	{"count", required_argument, NULL, 'c'},
	{"pid", required_argument, NULL, 'p'},
	{"shell", no_argument, NULL, 's'},
	{"verbose", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hVC:c:p:sv";

unsigned int cnt, count = 100;
unsigned int verbose;
unsigned int use_shell;
char *command, cmdstr[64];
pid_t pid;

void ShowHelp(void) {
	printf("setpid options:\n"
		"\t-h, --help\n"
		"\t-V, --version\n"
		"\t-C, --command \"COMMAND\"\n"
		"\t-c, --count NUM\n"
		"\t-p, --pid\n"
		"\t-s, --shell\n"
		"\t-v, --verbose\n");
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		ShowHelp();
		return EINVAL;
	}

	int c;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1) break;
		switch (c) {
		case 'h':
			ShowHelp();
			exit(0);
		case 'V':
			printf("setpid %s\n", setpid_version_string);
			exit(0);
		case 'C':
			command = (char *)malloc(strlen(optarg)+1+2);
			sprintf(command, "%s &", optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'p':
			pid = (pid_t)atoi(optarg);
			break;
		case 's':
			use_shell = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			fprintf(stderr, "setpid error: Unknown option: %d<%c>\n", c, (char)c);
			break;
		}
	}

	if (use_shell) {
		if (verbose)
			sprintf(cmdstr, "for i in `seq 1 %u`; do echo \"#$i: $$\"; done", count);
		else
			sprintf(cmdstr, "for i in `seq 1 %u`; do true; done", count);
		if (system(cmdstr) == 2)
			exit(0);
		exit(0);
	}

	if (pid) {
		pid_t current_pid = getpid();
		if (current_pid > pid) {
			while (1) {
				current_pid = fork();
				if (current_pid == -1) {
					if (verbose)
						printf("#%u pid == -1\n", cnt++);
					exit(1);
				}
				else if (current_pid != 0)
					kill(current_pid, SIGTERM);

				if (verbose)
					printf("#%u pid: %d\n", cnt++, current_pid);

				if (current_pid < pid)
					break;
			}
		}
		while (1) {
			current_pid = fork();
			if (current_pid == -1) {
				if (verbose)
					printf("#%u pid2 == -1\n", cnt++);
				exit(1);
			}
			else if (current_pid != 0)
				kill(current_pid, SIGKILL);

			if (verbose)
				printf("#%u pid2: %d\n", cnt++, current_pid);

			if (current_pid >= pid-1) {
				if (command)
					if (system(command) == 2)
						exit(0);
				break;
			}
		}
		exit(0);
	}
	else {
		pid_t pid = getpid();
		if (verbose)
			printf("#1: %d\n", pid);
		for (cnt = 1; cnt <= count; cnt++) {
			if (verbose) {
				sprintf(cmdstr, "echo \"#%u: $$\"", cnt);
				if (system(cmdstr) == 2)
					exit(0);
			}
			else
				if (system("true") == 2)
					exit(0);
		}
		if (command)
			if (system(command) == 2)
				exit(0);
	}

	return 0;
}

