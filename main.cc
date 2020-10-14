/**************************** **************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id);
void *consumer (void *id);

/* Definining job struct for the job queue */
struct Job {
  Job();
  Job(int duration);
  int _id;
  int _duration;
};

Job::Job() {}

Job::Job(int duration) : _duration(duration) {}

/* Defining data struct to pass into each thread */ 
struct ThreadData {
  ThreadData();
  ThreadData(Job* jobq, int qsize, int joblim, int sems, int id,
	     int* pro_index, int* con_index, timespec* jobWait);
  Job* _jobQueue;
  int _qSize;
  int _jobLim;
  int _sems;
  int _threadID;
  int* _proIndex;
  int* _conIndex;
  timespec* _jobWait;
};

ThreadData::ThreadData() {}

ThreadData::ThreadData(Job* jobq, int qsize, int joblim, int sems, int id,
		       int* pro_index, int* con_index, timespec* job_wait)
  : _jobQueue(jobq), _qSize(qsize), _jobLim(joblim), _sems(sems), _threadID(id),
    _proIndex(pro_index), _conIndex(con_index), _jobWait(job_wait) {}


int main (int argc, char **argv) {
  int arg_requirement = 5;
  if (argc != arg_requirement) {
    cout << "Need to specify 4 arguments in command line!\n";
    return(-1);
  }

  // init the constant data shared across threads  
  int const q_size = check_arg(argv[1]);
  int const job_lim = check_arg(argv[2]);
  int const pro_no = check_arg(argv[3]);
  int const con_no = check_arg(argv[4]);
  timespec* job_wait = new timespec;
  int const max_wait = 20;
  job_wait->tv_sec = max_wait;

  // init the variable data shared across threads
  Job* job_queue = new Job [q_size];
  int* pro_index = new int;
  int* con_index = new int;
  *pro_index = 0;
  *con_index = 0;
  
  // create the three semaphores that we need 
  int numsems = 3;
  int sems = sem_create(SEM_KEY, numsems);
  if(sems == -1) {
    cerr << "Couldn't create semaphore key!\n";
    return -1;
  }

  // initialising 'mutex' semaphore (lock for the critical section)
  if(sem_init(sems, 0, 1) == -1) {
    cerr << "Couldn't initialise 'mutex' semaphore!\n"; 
    return -1;
  }

  // initialising 'space' semaphore (ensure job queue isn't full) 
  if(sem_init(sems, 1, q_size) == -1) {
    cerr << "Couldn't initialise 'space' semaphore!\n";
    return -1;
  }

  // initialising 'items' semaphore (ensure job queue isn't empty) 
  if(sem_init(sems, 2, 0) == -1) {
    cerr << "Couldn't initialise 'item' semaphore!\n";
    return -1;
  }

  // create array of data (shared and individual) for each thread
  pthread_t pro_threads [pro_no], con_threads [con_no];
  ThreadData pro_data [pro_no], con_data [con_no];
  for (int i = 0; i < pro_no; i++)
    pro_data[i] = ThreadData(job_queue, q_size, job_lim, sems, i+1,
			     pro_index, con_index, job_wait);
  for (int i = 0; i < con_no; i++) 
    con_data[i] = ThreadData(job_queue, q_size, job_lim, sems, i+1,
			     pro_index, con_index, job_wait);  

  // create indiviudal thread for each producer and consumer 
  for (int i = 0; i < pro_no; i++) {
    if (pthread_create(&pro_threads[i], NULL, producer, (void*) &pro_data[i]) != 0) {
      cerr << "Error: Failed in creating a producer thread!\n";
      return -1;
    }
  }
  for (int i = 0; i < con_no; i++) {
    if (pthread_create(&con_threads[i], NULL, consumer, (void*) &con_data[i]) != 0) {
      cerr << "Error: Failed in creating a consumer thread!\n";
      return -1;
    }
  }
  
  // ensure thread finds another to join after exiting 
  for (int i = 0; i < pro_no; i++) {
    if (pthread_join(pro_threads[i], NULL) != 0) {
      cerr << "Error: Could not join consumer thread!\n";
      return -1;
    }
  }
  for (int i = 0; i < con_no; i++) {
    if (pthread_join(con_threads[i], NULL) != 0) {
      cerr << "Error: Could not join consumer thread!\n";
      return -1;
    }
  }

  if(sem_close(sems) == -1) {
    cerr << "Error: Could not close semaphores!\n";
    return -1;
  }
   
  delete [] job_queue;
  delete pro_index;
  delete con_index;
  delete job_wait;

  pthread_exit(0);
  return 0;
}

void *producer (void *parameter) {
  auto shared_data = (ThreadData *) parameter;
  int job_count = 0;
  int max_stutter = 5;
  int max_duration = 10; 
  while (job_count < shared_data->_jobLim) {   
    int duration = rand() % max_duration + 1;    
    Job new_job(duration);    
    job_count++;
    // destroy producer if no space in job queue after waiting 20 seconds
    if (sem_wait(shared_data->_sems, 1, shared_data->_jobWait) == -1) {
      printf("Producer(%i): No more availability to generate jobs.\n",
	     shared_data->_threadID);
      pthread_exit(0);
    }
    if (sem_wait(shared_data->_sems, 0, NULL) == -1) { // lock 'mutex' semaphore
      cerr << "Could not down 'mutex' sempahore!\n";
      pthread_exit(0);
    }
    
    // add appropriate job to the queue
    new_job._id = *shared_data->_proIndex + 1;
    shared_data->_jobQueue[*shared_data->_proIndex] = new_job;
    // increment producer index according to cicular job queue behaviour 
    *shared_data->_proIndex = (*shared_data->_proIndex + 1) % shared_data->_qSize;

    if (sem_signal(shared_data->_sems, 0) == -1) { // unlock 'mutex' semaphore
      cerr << "Could not up 'mutex' semaphore!\n";
      pthread_exit(0);
    }
    if (sem_signal(shared_data->_sems, 2) == -1) { // up 'item' semaphore    
      cerr << "Could not up 'item' semaphore!\n";
      pthread_exit(0);
    }
    
    // announce production status outside lock to reduce critical section
    printf("Producer(%i): Job id %i with duration of %i.\n",
	   shared_data->_threadID, new_job._id, new_job._duration);
    sleep(rand() % max_stutter + 1); // wait before creating a new job
  }
  
  // destroy producer if job count limit reached
  printf("Producer(%i): No more jobs to generate.\n",
	 shared_data->_threadID);
  pthread_exit(0);
}

void *consumer (void *parameter) {
  auto shared_data = (ThreadData *) parameter;
  // consumers constantly look for jobs
  while (true) {
    // destroy consumer if no jobs to consume after waiting 20 seconds
    if (sem_wait(shared_data->_sems, 2, shared_data->_jobWait) == -1) {
      printf("Consumer(%i): No more jobs left.\n", shared_data->_threadID);
      pthread_exit(0);
    }
    if (sem_wait(shared_data->_sems, 0, NULL) == -1) { // lock 'mutex' semaphore
      cerr << "Could not down 'mutex' sempahore!\n";
      pthread_exit(0);
    }
    
    // fetch appropriate job info from the job queue
    auto job = shared_data->_jobQueue[*shared_data->_conIndex];
    // increment consumer index according to cicular job queue behaviour
    *shared_data->_conIndex = (*shared_data->_conIndex + 1) % shared_data->_qSize;

    if (sem_signal(shared_data->_sems, 0) == -1) { // unlock 'mutex' semaphore
      cerr << "Could not up 'mutex' semaphore!\n";
      pthread_exit(0);
    }
    if (sem_signal(shared_data->_sems, 1) == -1) { // up 'space' semaphore    
      cerr << "Could not up 'space' semaphore!\n";
      pthread_exit(0);
    }
    
    // announce job consumption outside of lock to reduce critical section
    printf("Consumer(%i): Job id %i executing - sleep duration of %i.\n",
	   shared_data -> _threadID, job._id, job._duration);
    // consuming the job appropriately
    sleep(job._duration);
    printf("Consumer(%i): Job id %i completed.\n",
	   shared_data -> _threadID, job._id);
  }
}

