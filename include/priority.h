#ifndef PRIORITY_H
#define PRIORITY_H

#include "scheduler.h"

/*
 * Prioridade nao preemptiva. Convencao (docs/cenarios-de-carga.md): menor
 * valor numerico == maior prioridade. Desempate (docs/decisoes-tecnicas.md):
 * ordem de entrada na fila de prontos e, por fim, menor PID.
 */
Scheduler priority_scheduler(void);

#endif
