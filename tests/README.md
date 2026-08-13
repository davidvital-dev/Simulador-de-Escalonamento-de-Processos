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

Para executar toda a suíte do gerador:

```bash
make test-workload
```

Esse alvo cobre as 100 seeds nos quatro cenários, estrutura das rajadas, perfis
experimentais, configurações personalizadas, impressão de debug e sanidade da
lista de seeds.

Para verificar o determinismo nos quatro cenários:

```bash
make test-workload-determinism
```

Para verificar PID, chegada, prioridade e sequências de rajadas:

```bash
make test-workload-bursts
```

Para verificar se os quatro perfis experimentais são distintos e coerentes:

```bash
make test-workload-scenarios
```

Para verificar configurações personalizadas e configurações inválidas:

```bash
make test-workload-config
```

Para verificar a impressão de depuração de cargas pequenas:

```bash
make test-workload-debug
```

Para verificar a lista fixa de 100 seeds experimentais:

```bash
make test-experiment-seeds
```

## Testes do motor da simulação

Para executar a suíte do motor:

```bash
make test-simulator
```

Ela cobre quatro grupos.

### Runtime e cópia profunda

```bash
make test-process-runtime
```

Valida reset dos campos mutáveis, restauração dos tempos restantes e cópia
profunda das rajadas.

### Estados, chegadas e E/S

```bash
make test-simulator-states
```

Valida `NEW -> READY -> RUNNING`, `RUNNING -> BLOCKED -> READY`, término,
chegadas em tempos diferentes, CPU ociosa e operações de E/S paralelas.

### Troca de contexto e preempção

```bash
make test-simulator-context
```

Valida o custo principal de 2 ticks, ausência de custo em `idle -> processo` e
o contrato de preempção usado por políticas como Round Robin.

### Integração com o gerador

```bash
make test-simulator-workload
```

Gera 1.000 processos em cada um dos quatro cenários, cria uma cópia profunda da
carga e verifica se o motor leva todos os processos até `FINISHED` sem alterar
os valores originais das rajadas.

## Testes de métricas

### Turnaround e tempo mínimo ideal

```bash
make test-metrics-turnaround
```

Valida `metrics_turnaround` e `metrics_ideal_time` contra valores calculados à
mão para 5 processos, incluindo um caso em que `remaining_time` diverge
propositalmente de `duration` para garantir que o tempo mínimo ideal nunca
leia os contadores decrementados durante a simulação.

## Testes de estatística

### Média, desvio padrão amostral, IC95% e tabela consolidada

```bash
make test-stats
```

Valida `scripts/stats.py` (`summarize` e `summarize_table`) com dados
sintéticos calculados à mão, incluindo o caso de valores idênticos (desvio
padrão amostral = 0, IC95% colapsa na própria média) e de amostra única
(desvio padrão amostral tratado como 0 em vez de lançar exceção), sem
depender do pipeline experimental nem de um CSV real.

## Testes de gráficos

### Gráficos comparativos com IC95%

```bash
make test-plots
```

Requer `matplotlib` (veja `requirements.txt`). Usa `stats.summarize_table`
sobre uma carga fictícia para gerar a tabela consolidada e valida
`scripts/plots.py`: confirma título, eixos (com unidade), legenda e ordem
dos algoritmos em cada figura, e que os 3 arquivos PNG obrigatórios
(turnaround médio, trocas de contexto, índice de Jain) são gravados como
PNG válido. Também grava os 3 gráficos em `results/` para conferência
visual manual.

## Suíte completa

```bash
make test
```

Executa os testes do gerador, do motor, das métricas, da estatística e
dos gráficos disponíveis no repositório.
