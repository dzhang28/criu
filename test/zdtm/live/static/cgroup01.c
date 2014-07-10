#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include "zdtmtst.h"

const char *test_doc	= "Check that empty cgroups are preserved";
const char *test_author	= "Tycho Andersen <tycho.andersen@canonical.com>";

char *dirname;
TEST_OPTION(dirname, string, "cgroup directory name", 1);
static const char *cgname = "zdtmtst";
static const char *subname = "subcg";
static const char *empty = "empty";

int main(int argc, char **argv)
{
	int cgfd, l, ret = 1;
	char aux[1024], paux[1024];
	FILE *cgf;
	struct stat st;

	test_init(argc, argv);

	if (mkdir(dirname, 0700) < 0) {
		err("Can't make dir");
		goto out;
	}

	sprintf(aux, "none,name=%s", cgname);
	if (mount("none", dirname, "cgroup", 0, aux)) {
		err("Can't mount cgroups");
		goto out_rd;
	}

	sprintf(paux, "%s/%s", dirname, subname);
	mkdir(paux, 0600);

	l = sprintf(aux, "%d", getpid());
	sprintf(paux, "%s/%s/tasks", dirname, subname);

	cgfd = open(paux, O_WRONLY);
	if (cgfd < 0) {
		err("Can't open tasks");
		goto out_rs;
	}

	l = write(cgfd, aux, l);
	close(cgfd);

	if (l < 0) {
		err("Can't move self to subcg");
		goto out_rs;
	}

	sprintf(paux, "%s/%s/%s", dirname, subname, empty);
	mkdir(paux, 0600);

	test_daemon();
	test_waitsig();

	cgf = fopen("/proc/self/mountinfo", "r");
	if (cgf == NULL) {
		fail("No mountinfo file");
		goto out_rs;
	}

	while (fgets(paux, sizeof(paux), cgf)) {
		char *s;

		s = strstr(paux, cgname);
		if (s) {
			sscanf(paux, "%*d %*d %*d:%*d %*s %s", aux);
			test_msg("found cgroup at %s\n", aux);
			sprintf(paux, "%s/%s/%s", aux, subname, empty);
			if (stat(paux, &st)) {
				fail("couldn't stat %s\n", paux);
				ret = -1;
				goto out_close;
			}

			if (!S_ISDIR(st.st_mode)) {
				fail("%s is not a directory\n", paux);
				ret = -1;
				goto out_close;
			}

			pass();
			ret = 0;
			goto out_close;
		}
	}

	fail("empty cgroup not found!\n");

out_close:
	fclose(cgf);

	sprintf(paux, "%s/%s/%s", dirname, subname, empty);
	rmdir(paux);
out_rs:
	sprintf(paux, "%s/%s", dirname, subname);
	rmdir(paux);
	umount(dirname);
out_rd:
	rmdir(dirname);
out:
	return ret;
}
