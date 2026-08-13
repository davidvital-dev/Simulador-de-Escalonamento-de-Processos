#!/usr/bin/env python3
"""Valida cobertura do dataset e detecção de valores inválidos.

Usa datasets sintéticos (o CSV real ainda não existe -- depende do
pipeline e do binário reais) construídos a partir de
`run_experiments.build_combinations`, para cobrir: dataset completo e
válido, combinação faltando, combinação duplicada, e linhas com NaN,
infinito, negativo onde não faz sentido e Jain fora de [0, 1] --
confirmando que cada função aponta exatamente o problema certo, sem
travar nas demais linhas.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import run_experiments as runner  # noqa: E402  (import depende do sys.path acima)
import validate_dataset as vd  # noqa: E402

SCENARIOS = ("balanced", "io-bound")
ALGORITHMS = ("fcfs", "rr")
SEEDS = (1, 2)


def _combinations() -> list[runner.Combination]:
    return runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)


def _row_for(combination: runner.Combination, **overrides: object) -> dict[str, object]:
    row: dict[str, object] = {
        "seed": combination.seed,
        "cenario": combination.scenario,
        "algoritmo": combination.algorithm,
        "n_processos": 1000,
        "turnaround_medio": 100.0 + combination.seed,
        "trocas_contexto": 50 + combination.seed,
        "jain_slowdown": 0.9,
    }
    row.update(overrides)
    return row


def test_coverage_passes_on_complete_valid_dataset() -> None:
    combinations = _combinations()
    rows = [_row_for(c) for c in combinations]

    report = vd.check_dataset_coverage(rows, SCENARIOS, ALGORITHMS, SEEDS)

    assert report.is_complete
    assert report.missing == ()
    assert report.duplicated == ()


def test_coverage_detects_missing_combination() -> None:
    combinations = _combinations()
    missing_combination = combinations[0]
    rows = [_row_for(c) for c in combinations if c != missing_combination]

    report = vd.check_dataset_coverage(rows, SCENARIOS, ALGORITHMS, SEEDS)

    assert report.missing == (missing_combination,)
    assert report.duplicated == ()
    assert not report.is_complete


def test_coverage_detects_duplicated_combination() -> None:
    combinations = _combinations()
    duplicated_combination = combinations[0]
    rows = [_row_for(c) for c in combinations] + [_row_for(duplicated_combination)]

    report = vd.check_dataset_coverage(rows, SCENARIOS, ALGORITHMS, SEEDS)

    # todas as combinacoes esperadas apareceram pelo menos uma vez.
    assert report.missing == ()
    assert report.duplicated == ((duplicated_combination, 2),)
    assert not report.is_complete


def test_values_pass_on_valid_rows() -> None:
    rows = [_row_for(c) for c in _combinations()]

    assert vd.check_dataset_values(rows) == []


def test_values_detects_nan_infinite_negative_and_jain_out_of_range() -> None:
    combinations = _combinations()
    rows = [
        _row_for(combinations[0], turnaround_medio="nan"),
        _row_for(combinations[1], trocas_contexto="inf"),
        _row_for(combinations[2], turnaround_medio=-5.0),
        _row_for(combinations[3], jain_slowdown=1.5),
    ]

    invalid = vd.check_dataset_values(rows)

    assert len(invalid) == 4
    assert invalid[0].issues == (vd.FieldIssue("turnaround_medio", "nan", "nan"),)
    assert invalid[1].issues == (vd.FieldIssue("trocas_contexto", "inf", "infinito"),)
    assert invalid[2].issues == (vd.FieldIssue("turnaround_medio", -5.0, "negativo"),)
    assert invalid[3].issues == (
        vd.FieldIssue("jain_slowdown", 1.5, "jain_fora_da_faixa"),
    )

    # cada linha invalida sabe a qual combinacao cenario+algoritmo+seed
    # pertence, para o pipeline decidir o que reexecutar.
    for row_issues, combination in zip(invalid, combinations[:4]):
        assert row_issues.combination == combination


def test_values_detects_non_numeric_and_negative_jain() -> None:
    combinations = _combinations()
    rows = [
        _row_for(combinations[0], trocas_contexto="nao-e-numero"),
        _row_for(combinations[1], jain_slowdown=-0.1),
    ]

    invalid = vd.check_dataset_values(rows)

    assert invalid[0].issues == (
        vd.FieldIssue("trocas_contexto", "nao-e-numero", "nao_numerico"),
    )
    # jain negativo cai na checagem de faixa (mais especifica), nao na
    # checagem generica de "negativo" usada por turnaround/trocas.
    assert invalid[1].issues == (
        vd.FieldIssue("jain_slowdown", -0.1, "jain_fora_da_faixa"),
    )


def test_values_reports_every_issue_in_a_row_with_multiple_problems() -> None:
    combination = _combinations()[0]
    row = _row_for(combination, turnaround_medio=float("nan"), trocas_contexto=-3)

    row_issues = vd.check_row_values(row)

    fields_with_issues = {issue.field for issue in row_issues.issues}
    assert fields_with_issues == {"turnaround_medio", "trocas_contexto"}


def main() -> None:
    test_coverage_passes_on_complete_valid_dataset()
    test_coverage_detects_missing_combination()
    test_coverage_detects_duplicated_combination()
    test_values_pass_on_valid_rows()
    test_values_detects_nan_infinite_negative_and_jain_out_of_range()
    test_values_detects_non_numeric_and_negative_jain()
    test_values_reports_every_issue_in_a_row_with_multiple_problems()

    print(
        "OK: cobertura do dataset (faltantes/duplicadas) e deteccao de "
        "valores invalidos (NaN, infinito, negativos, Jain fora da "
        "faixa) validadas."
    )


if __name__ == "__main__":
    main()
