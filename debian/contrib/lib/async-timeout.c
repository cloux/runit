/* Runs specified command, and detach it from terminal, if it is still
 * running after DURATION.
 *
 * This utility is written specially for needs of integration `runit'
 * init system into Debian, so DURATION is hardcoded
 */

#define DURATION 90 /* seconds */

#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define write2(x) write(2, x "", sizeof(x) - 1)

static sig_atomic_t child_finished = 0;
static void signal_handler(int num) {
	if (num == SIGCHLD)
		child_finished = 1;
}

int main(int argc, char **argv) {
	pid_t pid;

	signal(SIGCHLD, &signal_handler);
	signal(SIGALRM, &signal_handler);

	pid = fork();
	if (pid == -1) {
		write2("async-timeout: fork() failed\n");
		return 1;
	}

	if (argc < 2) {
		write2("usage: async-timeout prog [args ...]\n");
		return 1;
	}

	if (pid == 0) { /* child */
		execvp(argv[1], argv + 1);
		write2("async-timeout: exec() failed\n");
		return 1;
	} else { /* parent */
		alarm(DURATION);
		pause(); /* wait for signal, either SIGCHLD or SIGALRM */
		if (child_finished) {
			int status;
			wait(&status);
			if (WIFEXITED(status)) {
				return WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				return 128 + WTERMSIG(status);
			} else { /* should not happen */
				assert("panic: unaccessible code" && 0);
				return 128;
			}
		} else {
			/* Child is reparented by pid1 */
			return 0;
		}
	};
	assert("panic: unaccessible code" && 0);

	return 0;
}
