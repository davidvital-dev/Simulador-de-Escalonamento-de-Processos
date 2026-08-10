#ifndef PROCESS_H
#define PROCESS_H

typedef enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_FINISHED
} ProcessState;

/*
 * Estrutura inicial propositalmente mínima.
 * David e Carlos devem fechar juntos o contrato definitivo antes de
 * adicionar arrays de rajadas e campos de simulação.
 */
typedef struct {
    int pid;
    int arrival_time;
    int priority;
    ProcessState state;
} Process;

#endif
