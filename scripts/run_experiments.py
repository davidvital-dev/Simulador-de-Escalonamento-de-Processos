#!/usr/bin/env python3
"""Automação dos experimentos principais: cenário x seed x algoritmo.

O binário real aceita `--scenario/--seed/--algorithm/--processes`; este runner
monta essa linha de comando para cada combinação. A orquestração continua sendo
testada isoladamente contra `scripts/fake_simulator.py`, enquanto
`tests/test_main_cli.py` valida a mesma interface no simulador real.

Não calcula estatística nem gera gráficos (isso já existe em
`scripts/stats.py` e `scripts/plots.py`); a única responsabilidade deste
módulo é orquestrar as execuções, capturar sucesso/falha e permitir
retomar só o que falta.

Convenção de progresso em disco (permite retomar entre execuções
separadas do script, sem precisar de um arquivo de estado à parte): uma
combinação é considerada bem-sucedida quando existe um arquivo não-vazio
em `<output_dir>/raw/<slug>.csv` -- o stdout exato daquela execução. Uma
combinação sem esse arquivo é "faltante", seja porque nunca rodou ou
porque a última tentativa falhou; rodar de novo tenta só essas.
"""

from __future__ import annotations

import argparse
import csv
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SEEDS_FILE = REPO_ROOT / "config" / "experiment-seeds.txt"
DEFAULT_OUTPUT_DIR = REPO_ROOT / "results" / "experiments"

# Mesmos identificadores usados por workload_scenario_name em src/workload.c
# e pelos escalonadores obrigatorios da issue #6.
SCENARIOS = ("balanced", "io-bound", "cpu-bound", "priority-imbalanced")
ALGORITHMS = ("fcfs", "rr", "priority", "proposed")

DEFAULT_PROCESSES = 1000


@dataclass(frozen=True)
class Combination:
    scenario: str
    algorithm: str
    seed: int


@dataclass(frozen=True)
class FailureRecord:
    combination: Combination
    exit_code: int
    stderr: str


@dataclass(frozen=True)
class RunSummary:
    # Todas as combinacoes confirmadas com sucesso apos esta chamada,
    # incluindo as que ja estavam prontas antes (puladas por retomada).
    succeeded: tuple[Combination, ...]
    # Combinacoes que foram tentadas nesta chamada e falharam agora.
    failed: tuple[FailureRecord, ...]


def build_combinations(
    scenarios: Sequence[str], algorithms: Sequence[str], seeds: Sequence[int],
) -> list[Combination]:
    return [
        Combination(scenario, algorithm, seed)
        for scenario in scenarios
        for algorithm in algorithms
        for seed in seeds
    ]


def load_seeds(path: str | Path, limit: int | None = None) -> list[int]:
    lines = [
        line.strip()
        for line in Path(path).read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    seeds = [int(line, 10) for line in lines]
    return seeds if limit is None else seeds[:limit]


def combination_slug(combination: Combination) -> str:
    return f"{combination.scenario}__{combination.algorithm}__seed{combination.seed}"


def raw_output_path(output_dir: Path, combination: Combination) -> Path:
    return output_dir / "raw" / f"{combination_slug(combination)}.csv"


def has_succeeded(output_dir: str | Path, combination: Combination) -> bool:
    path = raw_output_path(Path(output_dir), combination)
    return path.is_file() and path.stat().st_size > 0


def missing_combinations(
    combinations: Sequence[Combination], output_dir: str | Path,
) -> list[Combination]:
    """Combinações sem saída bem-sucedida em `output_dir`: faltantes ou
    cuja última tentativa falhou -- indistinguíveis por design, já que
    ambas precisam ser (re)tentadas."""
    output_dir = Path(output_dir)
    return [c for c in combinations if not has_succeeded(output_dir, c)]


def build_command(
    binary: str | Path, combination: Combination, process_count: int,
) -> list[str]:
    binary_path = Path(binary)
    command = [str(binary_path)]
    if binary_path.suffix.lower() == ".py":
        command.insert(0, sys.executable)

    return command + [
        "--scenario", combination.scenario,
        "--algorithm", combination.algorithm,
        "--seed", str(combination.seed),
        "--processes", str(process_count),
    ]


def run_combination(
    binary: str | Path,
    combination: Combination,
    process_count: int,
    *,
    timeout: float | None = None,
    extra_env: Mapping[str, str] | None = None,
) -> subprocess.CompletedProcess:
    env = None
    if extra_env is not None:
        env = dict(os.environ)
        env.update(extra_env)

    return subprocess.run(
        build_command(binary, combination, process_count),
        capture_output=True, text=True, timeout=timeout, env=env,
    )


def _write_failures_log(output_dir: Path, failures: Sequence[FailureRecord]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    with (output_dir / "failures.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["seed", "cenario", "algoritmo", "codigo_saida", "stderr"])
        for failure in failures:
            writer.writerow([
                failure.combination.seed,
                failure.combination.scenario,
                failure.combination.algorithm,
                failure.exit_code,
                failure.stderr,
            ])


def run_all(
    binary: str | Path,
    combinations: Sequence[Combination],
    output_dir: str | Path,
    *,
    process_count: int = DEFAULT_PROCESSES,
    resume: bool = True,
    timeout: float | None = None,
    extra_env: Mapping[str, str] | None = None,
) -> RunSummary:
    """Executa cada combinação faltante contra `binary` via subprocess.

    Com `resume=True` (padrão), combinações já bem-sucedidas (segundo
    `has_succeeded`) são puladas -- não são reexecutadas. Com
    `resume=False`, todas as combinações são tentadas de novo,
    sobrescrevendo qualquer saída anterior.

    O log de falhas em `<output_dir>/failures.csv` reflete só a chamada
    atual: uma combinação que falhou antes e teve sucesso agora não
    aparece mais nele.
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    to_run = missing_combinations(combinations, output_dir) if resume else list(combinations)
    already_done = [c for c in combinations if c not in to_run]

    succeeded: list[Combination] = list(already_done)
    failed: list[FailureRecord] = []

    for combination in to_run:
        result = run_combination(
            binary, combination, process_count,
            timeout=timeout, extra_env=extra_env,
        )

        if result.returncode == 0:
            raw_path = raw_output_path(output_dir, combination)
            raw_path.parent.mkdir(parents=True, exist_ok=True)
            raw_path.write_text(result.stdout, encoding="utf-8")
            succeeded.append(combination)
        else:
            failed.append(FailureRecord(combination, result.returncode, result.stderr))

    _write_failures_log(output_dir, failed)

    return RunSummary(succeeded=tuple(succeeded), failed=tuple(failed))


def format_summary(total: int, summary: RunSummary) -> str:
    lines = [
        f"{len(summary.succeeded)}/{total} combinacoes com sucesso.",
        f"{len(summary.failed)} combinacao(oes) falharam nesta execucao.",
    ]
    for failure in summary.failed:
        combination = failure.combination
        lines.append(
            f"  FALHA seed={combination.seed} cenario={combination.scenario} "
            f"algoritmo={combination.algorithm} exit_code={failure.exit_code}"
        )
    return "\n".join(lines)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--binary", required=True,
        help="executavel a chamar por combinacao (o simulador real ou "
             "scripts/fake_simulator.py para testes)",
    )
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR))
    parser.add_argument("--scenarios", nargs="+", default=list(SCENARIOS))
    parser.add_argument("--algorithms", nargs="+", default=list(ALGORITHMS))
    parser.add_argument("--seeds-file", default=str(DEFAULT_SEEDS_FILE))
    parser.add_argument(
        "--limit-seeds", type=int, default=None,
        help="usa so as N primeiras seeds do arquivo (util para teste rapido)",
    )
    parser.add_argument("--processes", type=int, default=DEFAULT_PROCESSES)
    parser.add_argument(
        "--force", action="store_true",
        help="reexecuta todas as combinacoes, mesmo as ja bem-sucedidas",
    )
    parser.add_argument("--timeout", type=float, default=None)
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    seeds = load_seeds(args.seeds_file, limit=args.limit_seeds)
    combinations = build_combinations(args.scenarios, args.algorithms, seeds)

    summary = run_all(
        args.binary, combinations, args.output_dir,
        process_count=args.processes, resume=not args.force, timeout=args.timeout,
    )

    print(format_summary(len(combinations), summary))

    return 0 if not summary.failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
