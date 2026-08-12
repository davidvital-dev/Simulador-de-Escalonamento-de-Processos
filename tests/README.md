# Testes

Antes de executar cargas com 1.000 processos, validar manualmente cargas pequenas (3 a 20 processos).

Casos mínimos:

- processo com uma única rajada de CPU;
- chegadas em tempos diferentes;
- CPU -> E/S -> CPU;
- dois processos em E/S simultaneamente;
- CPU ociosa;
- custo de troca de contexto;
- FCFS sem preempção;
- Round Robin com quantum pequeno;
- prioridade não preemptiva;
- determinismo da seed;
- métricas calculadas à mão.

## Teste do gerador de cargas

Para verificar o determinismo nos quatro cenários:

```bash
make test-workload-determinism
```

Para verificar PID, chegada, prioridade e sequências de rajadas:

```bash
make test-workload-bursts
```
