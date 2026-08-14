#!/usr/bin/env python3
"""Validação do dataset consolidado: cobertura e valores inválidos.

Módulo isolado, sem depender do binário real nem de rodar o pipeline:
opera sobre linhas já carregadas do CSV consolidado (dicts com as mesmas
colunas exportadas pelo simulador em C -- veja `run_experiments.py`),
então serve tanto para os dados fictícios de hoje quanto para o CSV real
depois, sem mudanças.

Reaproveita `Combination`/`build_combinations` de `run_experiments.py`
(mesma noção de "combinação cenário x algoritmo x seed") e
`METRIC_COLUMNS` de `stats.py` (mesmas 3 métricas obrigatórias) em vez de
redefinir essas convenções aqui.

Convenção do índice de Jain seguida: intervalo `[0, 1]`, a mesma usada
por `metrics.c` (`jain_accumulator_index`, sempre em `[0, 1]` pela
desigualdade de Cauchy-Schwarz), `stats.py` e `plots.py` (rótulo do eixo
"0-1").
"""

from __future__ import annotations

import math
from collections import Counter
from dataclasses import dataclass
from typing import Iterable, Mapping, Sequence

from run_experiments import Combination, build_combinations
from stats import METRIC_COLUMNS

# turnaround_medio e trocas_contexto nunca podem ser negativos.
# jain_slowdown tem sua propria checagem de faixa (nao so negatividade).
NON_NEGATIVE_FIELDS = ("turnaround_medio", "trocas_contexto")
JAIN_FIELD = "jain_slowdown"
JAIN_RANGE = (0.0, 1.0)


def _combination_from_row(row: Mapping[str, object]) -> Combination:
    return Combination(
        scenario=str(row["cenario"]),
        algorithm=str(row["algoritmo"]),
        seed=int(row["seed"]),
    )


@dataclass(frozen=True)
class CoverageReport:
    """`missing`: combinações esperadas ausentes do dataset.
    `duplicated`: combinações esperadas que aparecem mais de uma vez,
    junto da contagem encontrada (combinação, quantidade)."""

    missing: tuple[Combination, ...]
    duplicated: tuple[tuple[Combination, int], ...]

    @property
    def is_complete(self) -> bool:
        return not self.missing and not self.duplicated


def check_dataset_coverage(
    rows: Iterable[Mapping[str, object]],
    scenarios: Sequence[str],
    algorithms: Sequence[str],
    seeds: Sequence[int],
) -> CoverageReport:
    """Confere se cada combinação cenário x algoritmo x seed esperada
    (produto de `scenarios`, `algorithms`, `seeds`) aparece exatamente
    uma vez em `rows`. Combinações fora do produto esperado (cenário,
    algoritmo ou seed que não foi pedido) não são reportadas -- só a
    presença, ausência ou duplicação das combinações esperadas.
    """
    expected = build_combinations(scenarios, algorithms, seeds)

    counts: Counter[Combination] = Counter()
    for row in rows:
        counts[_combination_from_row(row)] += 1

    missing = tuple(
        combination for combination in expected if counts[combination] == 0
    )
    duplicated = tuple(
        (combination, counts[combination])
        for combination in expected
        if counts[combination] > 1
    )

    return CoverageReport(missing=missing, duplicated=duplicated)


@dataclass(frozen=True)
class FieldIssue:
    field: str
    value: object
    # "nao_numerico" | "nan" | "infinito" | "negativo" | "jain_fora_da_faixa"
    reason: str


@dataclass(frozen=True)
class RowIssues:
    row_index: int
    combination: Combination | None
    issues: tuple[FieldIssue, ...]


def check_row_values(row: Mapping[str, object], row_index: int = 0) -> RowIssues:
    """Verifica os campos numéricos obrigatórios (`stats.METRIC_COLUMNS`)
    de uma linha: não-numérico, NaN, infinito, negativo em campos que
    nunca podem ser negativos (`NON_NEGATIVE_FIELDS`), e `jain_slowdown`
    fora de `JAIN_RANGE`. Aceita valores já numéricos ou em string (como
    vem de `csv.DictReader`).
    """
    issues: list[FieldIssue] = []

    for field in METRIC_COLUMNS:
        raw_value = row[field]
        try:
            value = float(raw_value)
        except (TypeError, ValueError):
            issues.append(FieldIssue(field, raw_value, "nao_numerico"))
            continue

        if math.isnan(value):
            issues.append(FieldIssue(field, raw_value, "nan"))
            continue
        if math.isinf(value):
            issues.append(FieldIssue(field, raw_value, "infinito"))
            continue

        if field == JAIN_FIELD:
            lower, upper = JAIN_RANGE
            if not (lower <= value <= upper):
                issues.append(FieldIssue(field, raw_value, "jain_fora_da_faixa"))
        elif field in NON_NEGATIVE_FIELDS and value < 0:
            issues.append(FieldIssue(field, raw_value, "negativo"))

    try:
        combination = _combination_from_row(row)
    except (KeyError, ValueError):
        combination = None

    return RowIssues(row_index=row_index, combination=combination, issues=tuple(issues))


def check_dataset_values(rows: Sequence[Mapping[str, object]]) -> list[RowIssues]:
    """Aplica `check_row_values` a cada linha; retorna só as inválidas
    (com pelo menos um `FieldIssue`), na ordem original do dataset."""
    invalid: list[RowIssues] = []
    for index, row in enumerate(rows):
        row_issues = check_row_values(row, row_index=index)
        if row_issues.issues:
            invalid.append(row_issues)
    return invalid
