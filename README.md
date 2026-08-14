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
