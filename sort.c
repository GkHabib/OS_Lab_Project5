#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


int
main(int argc, char *argv[])
{
  shm_open(1,2,3);
  shm_attach(4);
  shm_close(5);
  exit();
}
