#ifndef FCFS_H
#define FCFS_H

#include "scheduler.h"

/* FCFS nao preemptivo: sempre pega o processo que esta a mais tempo na fila. */
Scheduler fcfs_scheduler(void);

#endif
