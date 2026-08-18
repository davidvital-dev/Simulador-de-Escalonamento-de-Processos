# Simulador de Escalonamento de Processos

Projeto da Unidade 3 da disciplina de Sistemas Operacionais.

## Equipe

- David
- Carlos
- Levi
- Henrique

## Objetivo

Desenvolver, em C, um simulador de escalonamento de processos capaz de gerar cargas de trabalho reprodutíveis por seed, executar algoritmos clássicos e comparar seus resultados com um algoritmo próprio da equipe.

## Algoritmos obrigatórios

- FCFS (First-Come, First-Served)
- Round Robin, com quantum configurável
- Prioridade não preemptiva
- Algoritmo próprio da equipe

## Cenários obrigatórios

1. Aleatório equilibrado
2. I/O-bound
3. CPU-bound / processos longos
4. Prioridades desbalanceadas

## Métricas principais

- Turnaround médio
- Trocas de contexto
- Índice de Jain aplicado ao slowdown

Os experimentos principais devem usar, no mínimo, 1.000 processos por execução e 100 seeds por cenário, com as mesmas cargas para todos os algoritmos.

## Estrutura do repositório

```text
include/             Headers compartilhados
src/                 Motor, processos, cargas e métricas
src/schedulers/      Algoritmos de escalonamento
scripts/             Execução em lote, consolidação e gráficos
tests/               Testes pequenos e reproduzíveis
results/             Resultados gerados (CSV/gráficos)
docs/                Decisões técnicas e planejamento da equipe
```

## Responsabilidades principais

| Integrante | Responsabilidade técnica principal |
|---|---|
| David | Motor da simulação, estados, E/S, troca de contexto e integração |
| Carlos | Geração determinística por seed e quatro cenários |
| Levi | FCFS, Round Robin, Prioridade e liderança do algoritmo próprio |
| Henrique | Métricas em C, CSV, execução em lote, IC95% e gráficos |

Todos devem produzir código, testes e participar da análise/apresentação.

## Compilação

```bash
make
```

Executável gerado:

```bash
./build/simulator
```

No Windows, o executável é `build/simulator.exe`.

## Execução individual

Exemplo com o cenário equilibrado, FCFS, seed `1` e 1.000 processos:

```bash
./build/simulator --scenario balanced --algorithm fcfs --seed 1 --processes 1000
```

Valores aceitos:

- `--scenario`: `balanced`, `io-bound`, `cpu-bound` ou `priority-imbalanced`;
- `--algorithm`: `fcfs`, `rr`, `priority` ou `proposed`;
- `--seed`: inteiro entre `0` e `UINT_MAX`;
- `--processes`: inteiro positivo;
- `--quantum`: inteiro positivo, padrão `4`;
- `--context-switch-cost`: inteiro não negativo, padrão `2`;
- `--debug`: envia as transições da simulação para `stderr`.

A saída padrão contém exatamente uma linha CSV, sem cabeçalho:

```text
seed,cenario,algoritmo,n_processos,turnaround_medio,trocas_contexto,jain_slowdown
```

Ajuda completa:

```bash
./build/simulator --help
```

## Experimentos principais

O comando abaixo usa, por padrão, os quatro cenários, os quatro algoritmos, as
100 seeds de `config/experiment-seeds.txt` e 1.000 processos por execução:

```bash
python scripts/run_experiments.py --binary ./build/simulator --processes 1000
```

No Windows, substitua o binário por `build/simulator.exe`. O runner preserva as
execuções concluídas e, quando chamado novamente, tenta apenas as combinações
faltantes. Para consolidar, validar a cobertura, calcular IC95% e gerar os três
gráficos obrigatórios:

```bash
python scripts/analyze_results.py
```

Os resultados consolidados da execução principal já estão versionados em
[`results/experiments/summary.csv`](results/experiments/summary.csv), junto aos
gráficos finais de [turnaround médio](results/experiments/plots/turnaround_medio.png),
[trocas de contexto](results/experiments/plots/trocas_contexto.png) e
[índice de Jain](results/experiments/plots/jain_slowdown.png). Os CSVs brutos
das 1.600 execuções permanecem ignorados porque podem ser reproduzidos pelos
comandos acima.

Limpeza:

```bash
make clean
```

## Scripts em Python (estatística e gráficos)

Os scripts em `scripts/` (estatística e gráficos) dependem de `matplotlib`:

```bash
pip install -r requirements.txt
```

## Convenções técnicas definidas

As decisões detalhadas estão em `docs/decisoes-tecnicas.md`.
Os parâmetros ajustáveis do gerador estão em `docs/cenarios-de-carga.md`.

- tempo discreto;
- prioridade `0` como maior prioridade;
- processos modelados por rajadas CPU/E/S;
- chegadas geradas pela seed;
- E/S paralela, sem fila de dispositivo no modelo principal;
- custo de troca de contexto de `2` ticks, configurável;
- sem custo ao sair da CPU ociosa;
- quantum principal do Round Robin de `4` ticks, configurável;
- índice de Jain calculado internamente entre `0` e `1`.

## Regra de integração

Deve existir **um único motor de simulação**. Os escalonadores apenas decidem quem executar e, quando aplicável, quando preemptar. A lógica de CPU, E/S, estados e tempo não deve ser duplicada nos algoritmos.

A carga deve ser gerada uma única vez para cada seed/cenário e clonada para cada algoritmo.
