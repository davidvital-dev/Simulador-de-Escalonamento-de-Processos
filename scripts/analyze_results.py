#!/usr/bin/env python3
"""Consolidação estatística e gráficos: cola entre os módulos isolados.

Único módulo do pipeline (além de `run_experiments.py`) que faz I/O de
disco: `stats.py`, `validate_dataset.py` e `plots.py` recebem dados já
carregados em memória, para serem testáveis sem depender de arquivos
reais. Este módulo lê os CSVs brutos gravados por `run_experiments.py`
em `<output_dir>/raw/<slug>.csv` (um por combinação bem-sucedida, sem
cabeçalho, no formato exportado pelo simulador em C -- veja
`stats.py`): seed,cenario,algoritmo,n_processos,turnaround_medio,
trocas_contexto,jain_slowdown.

Grava a tabela consolidada (`stats.summarize_table`, uma linha por
cenário+algoritmo+métrica) em `<output_dir>/summary.csv`, incluindo a
coluna `n` -- o número efetivo de seeds válidas que entraram naquele
grupo (seeds que rodaram com sucesso E cujos valores passaram em
`validate_dataset.check_dataset_values`). É aqui que esse número fica
registrado e consultável, em vez de existir só em memória durante o
cálculo (issue #6: "Registrar o número efetivo de seeds válidas").
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path
from typing import Sequence

SCRIPTS_DIR = Path(__file__).resolve().parent
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import plots  # noqa: E402  (import depende do sys.path acima)
import run_experiments as runner  # noqa: E402
import stats  # noqa: E402
import validate_dataset as vd  # noqa: E402

RAW_ROW_FIELDS = (
    "seed", "cenario", "algoritmo", "n_processos",
    "turnaround_medio", "trocas_contexto", "jain_slowdown",
)

SUMMARY_FIELDS = (
    "cenario", "algoritmo", "metrica", "media", "desvio_padrao",
    "ic95_inferior", "ic95_superior", "n",
)


def load_raw_rows(output_dir: str | Path) -> list[dict[str, object]]:
    """Lê todos os CSVs brutos em `<output_dir>/raw/`, um por combinação
    bem-sucedida (ver `run_experiments.has_succeeded`), e devolve as
    linhas parseadas no formato usado por `stats.py`/`validate_dataset.py`.

    Combinações que falharam não têm arquivo em `raw/` (já estão
    registradas separadamente em `<output_dir>/failures.csv`, escrito
    por `run_experiments.py`) e por isso não contribuem linha nenhuma
    aqui -- é assim que uma seed inválida deixa de contar no `n` final.
    """
    raw_dir = Path(output_dir) / "raw"
    rows: list[dict[str, object]] = []

    if not raw_dir.is_dir():
        return rows

    for path in sorted(raw_dir.glob("*.csv")):
        text = path.read_text(encoding="utf-8").strip()
        if not text:
            continue
        for line in text.splitlines():
            values = line.split(",")
            if len(values) != len(RAW_ROW_FIELDS):
                raise ValueError(
                    f"{path}: esperava {len(RAW_ROW_FIELDS)} colunas, "
                    f"encontrou {len(values)}: {line!r}"
                )
            row = dict(zip(RAW_ROW_FIELDS, values))
            row["seed"] = int(row["seed"])
            row["n_processos"] = int(row["n_processos"])
            rows.append(row)

    return rows


def valid_rows_only(
    rows: Sequence[dict[str, object]], invalid: Sequence[vd.RowIssues],
) -> list[dict[str, object]]:
    """Remove de `rows` as linhas que rodaram com sucesso mas têm valores
    inválidos (NaN, infinito, negativo, Jain fora de [0, 1]) -- essas não
    contam como seeds efetivamente válidas."""
    invalid_indexes = {issues.row_index for issues in invalid}
    return [row for index, row in enumerate(rows) if index not in invalid_indexes]


def write_summary_csv(table: Sequence[dict[str, object]], path: str | Path) -> Path:
    """Grava a tabela consolidada em CSV, com cabeçalho -- inclui a
    coluna `n`, o número efetivo de seeds válidas por
    cenário+algoritmo+métrica."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=SUMMARY_FIELDS)
        writer.writeheader()
        for row in table:
            writer.writerow(row)

    return path


def format_report(
    total_expected: int,
    rows: Sequence[dict[str, object]],
    coverage: vd.CoverageReport,
    invalid: Sequence[vd.RowIssues],
    table: Sequence[dict[str, object]],
) -> str:
    """Resumo legível em texto: total de seeds válidas vs. esperadas,
    cobertura/valores inválidos, e o `n` por cenário+algoritmo (igual
    para as três métricas, já que vêm das mesmas linhas)."""
    total_valid = len(rows) - len(invalid)

    lines = [
        f"{total_valid}/{total_expected} seeds validas "
        f"({len(rows)} execucao(oes) com sucesso, {len(invalid)} com "
        "valores invalidos)."
    ]

    if coverage.missing:
        lines.append(f"{len(coverage.missing)} combinacao(oes) ausente(s) do dataset.")
    if coverage.duplicated:
        lines.append(f"{len(coverage.duplicated)} combinacao(oes) duplicada(s) no dataset.")

    lines.append("")
    lines.append("Seeds validas por cenario+algoritmo:")
    seen: set[tuple[object, object]] = set()
    for row in table:
        key = (row["cenario"], row["algoritmo"])
        if key in seen:
            continue
        seen.add(key)
        lines.append(f"  {row['cenario']}/{row['algoritmo']}: n={row['n']}")

    return "\n".join(lines)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results-dir", default=str(runner.DEFAULT_OUTPUT_DIR))
    parser.add_argument("--scenarios", nargs="+", default=list(runner.SCENARIOS))
    parser.add_argument("--algorithms", nargs="+", default=list(runner.ALGORITHMS))
    parser.add_argument("--seeds-file", default=str(runner.DEFAULT_SEEDS_FILE))
    parser.add_argument(
        "--limit-seeds", type=int, default=None,
        help="usa so as N primeiras seeds do arquivo (mesma convencao de run_experiments.py)",
    )
    parser.add_argument(
        "--summary-csv", default=None,
        help="default: <results-dir>/summary.csv",
    )
    parser.add_argument(
        "--plots-dir", default=None,
        help="default: <results-dir>/plots",
    )
    parser.add_argument("--no-plots", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    results_dir = Path(args.results_dir)
    seeds = runner.load_seeds(args.seeds_file, limit=args.limit_seeds)
    combinations = runner.build_combinations(args.scenarios, args.algorithms, seeds)

    rows = load_raw_rows(results_dir)
    coverage = vd.check_dataset_coverage(rows, args.scenarios, args.algorithms, seeds)
    invalid = vd.check_dataset_values(rows)
    table = stats.summarize_table(valid_rows_only(rows, invalid))

    summary_path = Path(args.summary_csv) if args.summary_csv else results_dir / "summary.csv"
    write_summary_csv(table, summary_path)

    print(format_report(len(combinations), rows, coverage, invalid, table))
    print(f"\nTabela consolidada gravada em {summary_path}")

    if not args.no_plots and table:
        plots_dir = Path(args.plots_dir) if args.plots_dir else results_dir / "plots"
        plots.plot_all_metrics(table, plots_dir)
        print(f"Graficos gerados em {plots_dir}")

    return 0 if (coverage.is_complete and not invalid) else 1


if __name__ == "__main__":
    raise SystemExit(main())
