// Copyright 2018 Dmitry Bogatov <KAction@gnu.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// This source file compiles to following binaries:
//
//  - /sbin/shutdown
//  - /sbin/halt
//  - /sbin/reboot

// They must work when /sbin/init is runit, but PID1 is not. It happens
// when bin:runit-init replaces bin:systemd-sysv or bin:sysvinit-core.
//
// Process of system rebooting/halting has nothing to do with PID1, it
// is all about reboot(2).
//
// reboot(8)/halt(8), provided by sysvinit, when invoked without -f,
// ask PID1 to terminate all processes. As last step, PID1 invokes
// /etc/rc0.d/K08halt or /etc/rc6.d/K08reboot, which, at their turn,
// invoke reboot(8)/halt(8) with -f option. With that option on,
// reboot(2) is called, causing Linux to reboot/halt.
//
// They are not needed for work in command line, since 'runit' uses
// another, more simple interface to system
// but they have been here

// /sbin/reboot binaries. Strictly speaking, they are not
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/reboot.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

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

/* Since we are forced to support more and more of command line options
 * expected by initscripts, let us make it more structured.
 */
struct config {
	enum shutdown_action action;
	bool force;
	bool wtmp_only;
};

/* Luckily, systemd supports sysvinit pipe interface, so there
 * is no need to write separate function interfacing systemd.
 */
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

static bool
syscall_shutdown(shutdown_action action)
{
	sync();
	switch (action) {
	case ACTION_REBOOT:
		reboot(RB_AUTOBOOT);
		break;
	case ACTION_HALT:
		reboot(RB_POWER_OFF);
		break;
	}
	return false;
}

static bool
runit_shutdown(shutdown_action action)
{
	char* args[] = {
		"/sbin/init", (action == ACTION_HALT) ? "0" : "6", NULL };
	execv("/sbin/init", args);
	return false;
}

static void
parse_command_line(struct config *cfg, int argc, char **argv)
{
	int i;

	cfg->action    = ACTION_HALT;
	cfg->force     = false;
	cfg->wtmp_only = false;

	if (strstr(argv[0], "reboot")) {
		cfg->action = ACTION_REBOOT;
	}

	for (i = 1; i != argc; ++i) {
		if (strcmp(argv[i], "-f"))
			cfg->force = true;
		if (strcmp(argv[i], "--force"))
			cfg->force = true;
		if (strcmp(argv[i], "-w"))
			cfg->wtmp_only = true;
		if (strcmp(argv[i], "--wtmp-only"))
			cfg->wtmp_only = true;
	}
}

int
main(int argc, char **argv)
{
	struct config cfg;
	bool ok;

	parse_command_line(&cfg, argc, argv);

	if (geteuid() != 0) {
		write2("shutdown: must be superuser.\n");
		return 2;
	}
	if (cfg.wtmp_only) {
		// No-Op: see #919699
		return 0;
	}

	if (cfg.force) {
		ok = syscall_shutdown(cfg.action);
	} else {
		ok = sysv_shutdown(cfg.action)
			|| runit_shutdown(cfg.action);
		sleep(15);
		/* System should be already down, but still. */
		syscall_shutdown(cfg.action);
	}
	return ok ? 0 : 1;
}
