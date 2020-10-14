/******************************************************************
 * The helper file that contains the following helper functions:
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 ******************************************************************/

# include "helper.h"

int check_arg (char *buffer) {
  int i, num = 0, temp = 0;
  if (strlen (buffer) == 0)
    return -1;
  for (i=0; i < (int) strlen (buffer); i++)
  {
    temp = 0 + buffer[i];
    if (temp > 57 || temp < 48)
      return -1;
    num += pow (10, strlen (buffer)-i-1) * (buffer[i] - 48);
  }
  return num;
}

int sem_create (key_t key, int num) {
  int id;
  // -1 if fail or - if succesful
  return id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL);
}

int sem_init (int id, int num, int value) {
  union semun semctl_arg;
  semctl_arg.val = value;
  return semctl (id, num, SETVAL, semctl_arg); // -1 if fail or 0 if succesful
}

int sem_wait (int id, short unsigned int num, timespec *wait_time) {
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };

  return semtimedop(id, op, 1, wait_time); // -1 if fail or 0 if succesful
}

int sem_signal (int id, short unsigned int num) {
  struct sembuf op[] = {
    {num, 1, SEM_UNDO}
  };
  return semop (id, op, 1); // -1 if fail or 0 if succesful
}

int sem_close (int id) {
  return semctl (id, 0, IPC_RMID, 0); // -1 if fail or 0 if succesful
}
