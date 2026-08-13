#ifndef FCFS_H
#define FCFS_H

#include "scheduler.h"

/*
 * FCFS nao preemptivo. A fila de prontos ja preserva a ordem de entrada
 * (E/S concluida antes de novas chegadas, dentro do mesmo tick), entao
 * basta selecionar sempre a posicao 0.
 */
Scheduler fcfs_scheduler(void);

#endif
