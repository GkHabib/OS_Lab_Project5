#include "types.h"

#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "sharedm.h"


int
main(int argc, char *argv[])
{
  shm_open(1, 1, ONLY_CHILD_CAN_ATTACH);
  shm_attach(1);
  int pid;
  if((pid = fork()))
  {
    if((pid = fork()))
    {
      wait();
      wait();
      shm_close(1);
      exit();
    }
    else
    {
      shm_attach(1);
      shm_close(1);
    }
  }
  else
  {
    shm_attach(1);
    shm_close(1);
  }
  // shm_open(1, 2, ONLY_OWNER_WRITE | ONLY_CHILD_CAN_ATTACH);
  // shm_attach(1);
}
