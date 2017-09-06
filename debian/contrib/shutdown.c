#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define write2(msg) write(2, msg, sizeof(msg))
typedef enum shutdown_action { ACTION_REBOOT, ACTION_HALT } shutdown_action;

/* Copied and slightly simplified from sysvinit-2.88dsf:/src/initreq.h */
struct sysv_request {
	int	magic;			/* Magic number                 */
	int	cmd;			/* What kind of request         */
	int	runlevel;		/* Runlevel to change to        */
	int	sleeptime;		/* Time between TERM and KILL   */
	char	unused[368];
};
#define SYSV_MAGIC              0x03091969
#define SYSV_CMD_RUNLVL         1
#define SYSV_FIFO               "/run/initctl"

static bool
sysv_shutdown(shutdown_action action)
{
	struct sysv_request request;
	request.magic = SYSV_MAGIC;
	request.cmd = SYSV_CMD_RUNLVL;

	switch (action) {
	case ACTION_REBOOT: request.runlevel = '6'; break;
	case ACTION_HALT:   request.runlevel = '0'; break;
	default: return false;
	};

	int fd = open(SYSV_FIFO, O_WRONLY);
	if (fd < 0) {
		return false;
	}
	bool ok = write(fd, &request, sizeof(request)) == sizeof(request);
	close(fd);
	return ok;
}


int
main(int argc, char **argv)
{
	shutdown_action action;
	if (argc == 1)
		action = ACTION_HALT;
	else if (argc == 2 && strcmp(argv[1], "0") == 0)
		action = ACTION_HALT;
	else if (argc == 2 && strcmp(argv[1], "6") == 0)
		action = ACTION_REBOOT;
	else {
		write2("Usage: shutdown [0|6]\n");
		return 3;
	}
	if (geteuid() != 0) {
		write2("shutdown: must be superuser.\n");
		return 2;
	}

	/* Luckily, systemd supports sysvinit pipe interface, so there
	 * is no need to write separate function interfacing systemd.
	 */
	bool via_sysv = sysv_shutdown(action);

	if (!via_sysv) {
		char* args[] = { "/sbin/init", (action == ACTION_HALT) ? "0" : "6", NULL };
		return execv("/sbin/init", args);
	}
}
