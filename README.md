# Producer-Consumer solutions using threads

Implementation of the producer-consumer scenario using POSIX threads, where each producer and consumer are running in a different thread. The shared data structure used will be a circular queue, which is implemented as an array. This array contains details of the job, where a job has a job id (which is one plus the location that they occupy in the array ⇒ job ids will start from 1) and a duration (which is the time taken to ‘process’ the job – for this coursework, processing a job will mean that the consumer thread will sleep for that duration).
