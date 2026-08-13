#!/usr/bin/env python3
"""Estatistica agregada dos resultados de experimentos.

Modulo isolado, sem I/O: recebe valores ja carregados em memoria e calcula
media, desvio padrao amostral e IC95%. Nao le arquivos, nao roda o
simulador e nao gera graficos -- isso fica para scripts/run_experiments.py
e scripts/analyze_results.py, que ainda dependem do pipeline de outros
integrantes.

As linhas esperadas pela funcao de agrupamento seguem o formato do CSV
exportado pelo simulador em C:
seed,cenario,algoritmo,n_processos,turnaround_medio,trocas_contexto,jain_slowdown
"""

from __future__ import annotations

import math
import statistics
from collections import defaultdict
from typing import Iterable, Mapping, NamedTuple, Sequence

METRIC_COLUMNS = ("turnaround_medio", "trocas_contexto", "jain_slowdown")


class SummaryStats(NamedTuple):
    mean: float
    stdev: float
    ci95_lower: float
    ci95_upper: float
    n: int


def summarize(values: Sequence[float]) -> SummaryStats:
    """Media, desvio padrao amostral (ddof=1) e IC95% de uma amostra.

    IC95 = media +- 1.96 * s / sqrt(n), com s = desvio padrao amostral.

    Com um unico valor, o desvio padrao amostral nao e definido (precisa de
    ao menos duas observacoes); tratamos esse caso como desvio 0 em vez de
    lancar excecao. Com valores identicos (n >= 2), o desvio padrao da
    exatamente 0 e o intervalo colapsa na propria media -- em nenhum dos
    dois casos ha divisao por zero, pois sqrt(n) so e zero se n for zero,
    e n == 0 e rejeitado explicitamente.
    """
    n = len(values)
    if n == 0:
        raise ValueError("summarize requer pelo menos um valor")

    mean = statistics.fmean(values)
    stdev = statistics.stdev(values) if n > 1 else 0.0
    margin = 1.96 * stdev / math.sqrt(n)

    return SummaryStats(
        mean=mean,
        stdev=stdev,
        ci95_lower=mean - margin,
        ci95_upper=mean + margin,
        n=n,
    )


def summarize_table(
    rows: Iterable[Mapping[str, object]],
    metrics: Sequence[str] = METRIC_COLUMNS,
) -> list[dict[str, object]]:
    """Agrupa linhas de execucao por cenario+algoritmo e resume cada metrica.

    `rows` aceita qualquer iteravel de mapeamentos com, no minimo, as
    chaves "cenario", "algoritmo" e as colunas em `metrics` (por exemplo,
    linhas de csv.DictReader sobre o CSV exportado pelo simulador, ou
    dicts ja construidos em memoria). Os valores das metricas sao
    convertidos para float aqui, entao tanto str quanto int/float servem.

    Retorna uma lista de dicts, uma linha por (cenario, algoritmo, metrica),
    com as colunas: cenario, algoritmo, metrica, media, desvio_padrao,
    ic95_inferior, ic95_superior, n. A ordem e deterministica (ordenada por
    cenario, algoritmo e metrica) para facilitar comparacao e testes.
    """
    grouped: dict[tuple[str, str], dict[str, list[float]]] = defaultdict(
        lambda: defaultdict(list)
    )

    for row in rows:
        key = (str(row["cenario"]), str(row["algoritmo"]))
        for metric in metrics:
            grouped[key][metric].append(float(row[metric]))

    table: list[dict[str, object]] = []
    for scenario, algorithm in sorted(grouped):
        for metric in metrics:
            values = grouped[(scenario, algorithm)][metric]
            summary = summarize(values)
            table.append({
                "cenario": scenario,
                "algoritmo": algorithm,
                "metrica": metric,
                "media": summary.mean,
                "desvio_padrao": summary.stdev,
                "ic95_inferior": summary.ci95_lower,
                "ic95_superior": summary.ci95_upper,
                "n": summary.n,
            })

    return table
