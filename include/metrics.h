#ifndef METRICS_H
#define METRICS_H

#include "process.h"

/* turnaround = finish_time - arrival_time. */
int metrics_turnaround(const Process *process);

/*
 * Tempo minimo ideal: soma das duracoes ORIGINAIS de todas as rajadas
 * (bursts[i].duration). Nunca usa remaining_time, pois esse campo e
 * decrementado durante a simulacao e deixaria de refletir a carga original.
 */
int metrics_ideal_time(const Process *process);

/* TODO(Henrique): definir RunMetrics e funções de cálculo/exportação. */
void metrics_placeholder(void);

#endif
