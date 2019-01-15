#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


int
main(int argc, char *argv[])
{
  shm_open(1,2,2);
  // shm_attach(1);
  shm_close(1);
  exit();
}
