/* Public domain. */

#include <fcntl.h>
#include "coe.h"

void coe(int fd)
{
  fcntl(fd,F_SETFD,1);
}
