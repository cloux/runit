#include <sys/types.h>
#include <utmp.h>

struct utmp ut;

int main(void) {
  char *s =ut.ut_user;
  return(0);
}
