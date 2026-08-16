#!/usr/bin/env python3
"""Valida a consolidacao final do pipeline: leitura dos CSVs brutos,
exclusao de linhas com valores invalidos e a tabela resumo com `n` --
o numero efetivo de seeds validas -- gravada em disco (issue #6:
"Registrar o numero efetivo de seeds validas").

Roda contra o binario fake (mesma convencao de test_run_experiments.py):
gera CSVs brutos de verdade via run_experiments.run_all com 10 seeds,
1 delas forcada a falhar (seed invalida real, via exit code != 0), e
confere que o `n` final bate com a contagem manual (10 - 1 = 9).
"""

import csv
import io
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import analyze_results as ar  # noqa: E402  (import depende do sys.path acima)
import run_experiments as runner  # noqa: E402

FAKE_SIMULATOR = Path(__file__).resolve().parents[1] / "scripts" / "fake_simulator.py"

SCENARIOS = ("balanced",)
ALGORITHMS = ("fcfs",)
SEEDS = tuple(range(1, 11))  # 10 seeds; nenhuma delas e' a 13 (default do fake)


def test_load_raw_rows_excludes_the_forced_failure() -> None:
    combinations = runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        summary = runner.run_all(
            FAKE_SIMULATOR, combinations, output_dir, process_count=100,
            extra_env={"FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS": "5"},
        )

        assert len(summary.failed) == 1
        assert summary.failed[0].combination.seed == 5

        rows = ar.load_raw_rows(output_dir)

        # 10 seeds - 1 falha forcada = 9 linhas lidas do disco.
        assert len(rows) == 9
        assert {row["seed"] for row in rows} == set(SEEDS) - {5}


def test_summary_csv_registers_the_effective_valid_seed_count() -> None:
    combinations = runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        runner.run_all(
            FAKE_SIMULATOR, combinations, output_dir, process_count=100,
            extra_env={"FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS": "5"},
        )

        rows = ar.load_raw_rows(output_dir)
        invalid = ar.vd.check_dataset_values(rows)
        assert invalid == []  # fake_simulator so falha via exit code, nunca com valor ruim

        table = ar.stats.summarize_table(ar.valid_rows_only(rows, invalid))
        summary_path = ar.write_summary_csv(table, output_dir / "summary.csv")
        assert summary_path.is_file()

        with summary_path.open(newline="", encoding="utf-8") as stream:
            summary_rows = list(csv.DictReader(stream))

        # 1 cenario x 1 algoritmo x 3 metricas obrigatorias = 3 linhas.
        assert len(summary_rows) == 3
        for row in summary_rows:
            assert row["cenario"] == "balanced"
            assert row["algoritmo"] == "fcfs"
            # numero efetivo de seeds validas: 10 pedidas - 1 falha
            # forcada = 9, batendo com a contagem manual esperada.
            assert int(row["n"]) == 9


def test_rows_with_invalid_values_do_not_count_as_valid_seeds() -> None:
    # todas as 10 seeds "rodam" com sucesso (exit code 0), mas uma delas
    # e' corrompida manualmente para simular um valor impossivel --
    # diferente da falha por exit code, esse tipo de seed invalida so e'
    # detectavel pelo conteudo, nao pelo runner.
    combinations = runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        summary = runner.run_all(FAKE_SIMULATOR, combinations, output_dir, process_count=100)
        assert summary.failed == ()

        corrupted = runner.Combination("balanced", "fcfs", 1)
        raw_path = runner.raw_output_path(output_dir, corrupted)
        # jain_slowdown fora da faixa valida [0, 1].
        raw_path.write_text("1,balanced,fcfs,100,101.00,51,1.50\n", encoding="utf-8")

        rows = ar.load_raw_rows(output_dir)
        assert len(rows) == 10  # todas as 10 seeds tem arquivo raw (nenhuma falhou por exit code)

        invalid = ar.vd.check_dataset_values(rows)
        assert len(invalid) == 1

        table = ar.stats.summarize_table(ar.valid_rows_only(rows, invalid))
        for row in table:
            # 10 seeds rodaram, mas so 9 tem valor valido.
            assert row["n"] == 9


def test_main_end_to_end_matches_the_manual_count() -> None:
    combinations = runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        runner.run_all(
            FAKE_SIMULATOR, combinations, output_dir, process_count=100,
            extra_env={"FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS": "7"},
        )

        seeds_file = output_dir / "seeds.txt"
        seeds_file.write_text("\n".join(str(seed) for seed in SEEDS), encoding="utf-8")

        stdout = io.StringIO()
        with redirect_stdout(stdout):
            exit_code = ar.main([
                "--results-dir", str(output_dir),
                "--scenarios", *SCENARIOS,
                "--algorithms", *ALGORITHMS,
                "--seeds-file", str(seeds_file),
                "--no-plots",
            ])

        # 1 seed forcada a falhar -> dataset fica incompleto -> exit code 1,
        # mesmo com a tabela consolidada gravada corretamente.
        assert exit_code == 1
        assert "9/10 seeds validas" in stdout.getvalue()

        summary_path = output_dir / "summary.csv"
        assert summary_path.is_file()
        with summary_path.open(newline="", encoding="utf-8") as stream:
            summary_rows = list(csv.DictReader(stream))
        assert len(summary_rows) == 3
        assert all(int(row["n"]) == 9 for row in summary_rows)


def main() -> None:
    test_load_raw_rows_excludes_the_forced_failure()
    test_summary_csv_registers_the_effective_valid_seed_count()
    test_rows_with_invalid_values_do_not_count_as_valid_seeds()
    test_main_end_to_end_matches_the_manual_count()

    print(
        "OK: leitura dos CSVs brutos, exclusao de valores invalidos e "
        "registro do numero efetivo de seeds validas (summary.csv) "
        "validados contra o binario fake."
    )


if __name__ == "__main__":
    main()
