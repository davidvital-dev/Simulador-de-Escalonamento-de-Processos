#!/usr/bin/env python3
"""Valida media, desvio padrao amostral, IC95% e a tabela consolidada.

Usa dados sinteticos (nao depende do pipeline nem de um CSV real, que
ainda nao existem) e recalcula os valores esperados manualmente, pela
propria formula do enunciado, para nao testar o modulo contra si mesmo.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import stats  # noqa: E402  (import depende do sys.path acima)


def test_summarize_matches_manual_calculation() -> None:
    values = [10.0, 12.0, 14.0]

    manual_mean = sum(values) / len(values)
    manual_sum_sq_dev = sum((x - manual_mean) ** 2 for x in values)
    manual_stdev = math.sqrt(manual_sum_sq_dev / (len(values) - 1))
    manual_margin = 1.96 * manual_stdev / math.sqrt(len(values))

    result = stats.summarize(values)

    assert math.isclose(result.mean, manual_mean, rel_tol=1e-9)
    assert math.isclose(result.stdev, manual_stdev, rel_tol=1e-9)
    assert math.isclose(result.ci95_lower, manual_mean - manual_margin, rel_tol=1e-9)
    assert math.isclose(result.ci95_upper, manual_mean + manual_margin, rel_tol=1e-9)
    assert result.n == 3


def test_summarize_identical_values_collapses_ci_to_mean() -> None:
    result = stats.summarize([5.0, 5.0, 5.0, 5.0])

    assert result.mean == 5.0
    assert result.stdev == 0.0
    assert result.ci95_lower == 5.0
    assert result.ci95_upper == 5.0
    assert result.n == 4
    assert not math.isnan(result.stdev)
    assert not math.isnan(result.ci95_lower)
    assert not math.isnan(result.ci95_upper)


def test_summarize_single_value_has_zero_stdev_without_crashing() -> None:
    # statistics.stdev exige >= 2 amostras; com uma unica amostra o desvio
    # padrao amostral nao e definido, entao tratamos como 0 em vez de
    # lancar excecao ou propagar NaN.
    result = stats.summarize([7.0])

    assert result.mean == 7.0
    assert result.stdev == 0.0
    assert result.ci95_lower == 7.0
    assert result.ci95_upper == 7.0
    assert result.n == 1


def test_summarize_rejects_empty_sequence() -> None:
    try:
        stats.summarize([])
    except ValueError:
        pass
    else:
        raise AssertionError("summarize([]) deveria levantar ValueError")


def test_summarize_table_groups_and_matches_manual_calculation() -> None:
    # Dados ficticios no formato das colunas do CSV: duas seeds por
    # cenario+algoritmo, dois algoritmos, um cenario.
    rows = [
        {
            "seed": 1, "cenario": "balanced", "algoritmo": "fcfs",
            "n_processos": 1000, "turnaround_medio": 100.0,
            "trocas_contexto": 50, "jain_slowdown": 0.90,
        },
        {
            "seed": 2, "cenario": "balanced", "algoritmo": "fcfs",
            "n_processos": 1000, "turnaround_medio": 110.0,
            "trocas_contexto": 60, "jain_slowdown": 0.92,
        },
        {
            "seed": 1, "cenario": "balanced", "algoritmo": "rr",
            "n_processos": 1000, "turnaround_medio": 95.0,
            "trocas_contexto": 200, "jain_slowdown": 0.97,
        },
        {
            "seed": 2, "cenario": "balanced", "algoritmo": "rr",
            "n_processos": 1000, "turnaround_medio": 105.0,
            "trocas_contexto": 210, "jain_slowdown": 0.95,
        },
    ]

    table = stats.summarize_table(rows)

    # 2 grupos (cenario+algoritmo) x 3 metricas = 6 linhas.
    assert len(table) == 6

    # turnaround_medio do grupo balanced+fcfs, calculado a mao:
    # media = (100 + 110) / 2 = 105.
    # variancia amostral = ((100-105)^2 + (110-105)^2) / (2-1) = 50.
    # desvio = sqrt(50) ~= 7.0710678.
    # margem = 1.96 * sqrt(50) / sqrt(2) = 1.96 * 5.0 = 9.8 (sqrt(50/2) = 5).
    first_row = table[0]
    assert first_row["cenario"] == "balanced"
    assert first_row["algoritmo"] == "fcfs"
    assert first_row["metrica"] == "turnaround_medio"
    assert math.isclose(first_row["media"], 105.0, rel_tol=1e-9)
    assert math.isclose(first_row["desvio_padrao"], math.sqrt(50), rel_tol=1e-9)
    assert math.isclose(first_row["ic95_inferior"], 105.0 - 9.8, rel_tol=1e-9)
    assert math.isclose(first_row["ic95_superior"], 105.0 + 9.8, rel_tol=1e-9)
    assert first_row["n"] == 2

    # ordem deterministica: fcfs vem antes de rr (ordem alfabetica).
    algorithms_in_order = [row["algoritmo"] for row in table]
    assert algorithms_in_order == ["fcfs", "fcfs", "fcfs", "rr", "rr", "rr"]


def test_summarize_table_accepts_csv_dictreader_style_strings() -> None:
    # csv.DictReader devolve todas as colunas como str; o modulo precisa
    # aceitar isso sem exigir conversao previa por parte do chamador.
    rows_as_strings = [
        {
            "seed": "1", "cenario": "balanced", "algoritmo": "fcfs",
            "n_processos": "1000", "turnaround_medio": "100.0",
            "trocas_contexto": "50", "jain_slowdown": "0.90",
        },
        {
            "seed": "2", "cenario": "balanced", "algoritmo": "fcfs",
            "n_processos": "1000", "turnaround_medio": "110.0",
            "trocas_contexto": "60", "jain_slowdown": "0.92",
        },
    ]

    table = stats.summarize_table(rows_as_strings)

    assert len(table) == 3
    turnaround_row = next(
        row for row in table if row["metrica"] == "turnaround_medio"
    )
    assert math.isclose(turnaround_row["media"], 105.0, rel_tol=1e-9)
    assert turnaround_row["n"] == 2


def main() -> None:
    test_summarize_matches_manual_calculation()
    test_summarize_identical_values_collapses_ci_to_mean()
    test_summarize_single_value_has_zero_stdev_without_crashing()
    test_summarize_rejects_empty_sequence()
    test_summarize_table_groups_and_matches_manual_calculation()
    test_summarize_table_accepts_csv_dictreader_style_strings()

    print("OK: media, desvio padrao amostral, IC95% e tabela consolidada validados.")


if __name__ == "__main__":
    main()
