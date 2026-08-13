#ifndef METRICS_H
#define METRICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "process.h"
#include "simulator.h"

/* turnaround = finish_time - arrival_time. */
int metrics_turnaround(const Process *process);

/*
 * Tempo minimo ideal: soma das duracoes ORIGINAIS de todas as rajadas
 * (bursts[i].duration). Nunca usa remaining_time, pois esse campo e
 * decrementado durante a simulacao e deixaria de refletir a carga original.
 */
int metrics_ideal_time(const Process *process);

/*
 * slowdown = turnaround / tempo_minimo_ideal.
 * Retorna -1.0 quando o tempo minimo ideal for <= 0 (carga invalida ou
 * processo sem rajadas), evitando divisao por zero.
 */
double metrics_slowdown(const Process *process);

typedef struct {
    unsigned int seed;
    char scenario[32];
    char algorithm[32];
    size_t process_count;
    double avg_turnaround;
    size_t context_switches;
    double jain_slowdown;
} RunMetrics;

/*
 * Calcula as metricas agregadas de uma execucao completa a partir do vetor
 * de processos ja simulado e do SimulationResult correspondente.
 *
 * Considera apenas processos com state == PROCESS_FINISHED. Processos cujo
 * metrics_slowdown for invalido (< 0, carga sem rajadas) sao ignorados no
 * calculo do turnaround medio e do indice de Jain, sem travar a funcao.
 *
 * out->process_count recebe o tamanho total da carga (o process_count de
 * entrada), independente de quantos processos entraram no calculo.
 * out->context_switches vem diretamente do contador oficial do motor
 * (result->context_switches).
 *
 * Retorna false, sem escrever em out, quando nao houver nenhum processo
 * finalizado valido.
 */
bool metrics_compute_run(const Process *processes, size_t process_count,
                         const SimulationResult *result, unsigned int seed,
                         const char *scenario, const char *algorithm,
                         RunMetrics *out);

/*
 * Escreve o cabeçalho do CSV, com uma quebra de linha ao final:
 * seed,cenario,algoritmo,n_processos,turnaround_medio,trocas_contexto,jain_slowdown
 */
void metrics_csv_write_header(FILE *stream);

/*
 * Escreve uma linha do CSV para uma execucao, na mesma ordem de colunas do
 * cabecalho. turnaround_medio usa 2 casas decimais; jain_slowdown usa 4.
 */
void metrics_csv_write_row(FILE *stream, const RunMetrics *metrics);

#endif
