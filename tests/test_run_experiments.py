#!/usr/bin/env python3
"""Valida o runner contra o binário fake, isolado do custo do simulador real.

Cobre: log de falhas com todos os campos exigidos (seed, cenário,
algoritmo, stderr, código de saída), e reexecução em modo "só o que
falta" -- combinações já bem-sucedidas não são repetidas; as que
falharam são tentadas de novo e, se passarem a funcionar, saem do log de
falhas.
"""

import csv
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import run_experiments as runner  # noqa: E402  (import depende do sys.path acima)

FAKE_SIMULATOR = Path(__file__).resolve().parents[1] / "scripts" / "fake_simulator.py"

SCENARIOS = ("balanced", "io-bound")
ALGORITHMS = ("fcfs", "rr")
SEEDS = (1, 2, 13)  # 13 sempre falha em fake_simulator.py (ALWAYS_FAIL_SEEDS)


def _build_test_combinations() -> list[runner.Combination]:
    return runner.build_combinations(SCENARIOS, ALGORITHMS, SEEDS)


def _raw_mtimes(output_dir: Path, combinations) -> dict[runner.Combination, float]:
    return {
        combination: runner.raw_output_path(output_dir, combination).stat().st_mtime_ns
        for combination in combinations
        if runner.has_succeeded(output_dir, combination)
    }


def test_failure_log_has_all_required_fields() -> None:
    combinations = _build_test_combinations()
    expected_failed = {c for c in combinations if c.seed == 13}
    expected_succeeded = set(combinations) - expected_failed

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        summary = runner.run_all(
            FAKE_SIMULATOR, combinations, output_dir, process_count=100,
        )

        assert set(summary.succeeded) == expected_succeeded
        assert {failure.combination for failure in summary.failed} == expected_failed

        for failure in summary.failed:
            assert failure.combination.seed == 13
            assert failure.exit_code != 0
            assert failure.stderr.strip(), "stderr da falha nao pode estar vazio"

        # log em disco, com as mesmas informacoes, no formato CSV com
        # cabecalho, para inspecao/depuracao fora do processo Python.
        failures_path = output_dir / "failures.csv"
        assert failures_path.is_file()
        with failures_path.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.DictReader(stream))

        assert len(rows) == len(expected_failed)
        for row in rows:
            assert row["seed"] == "13"
            assert row["cenario"] in SCENARIOS
            assert row["algoritmo"] in ALGORITHMS
            assert row["codigo_saida"] != "0"
            assert row["stderr"].strip()

        # cada combinacao bem-sucedida produziu a linha de CSV esperada.
        for combination in expected_succeeded:
            raw_path = runner.raw_output_path(output_dir, combination)
            fields = raw_path.read_text(encoding="utf-8").strip().split(",")
            seed, scenario, algorithm, processes = fields[0:4]
            assert int(seed) == combination.seed
            assert scenario == combination.scenario
            assert algorithm == combination.algorithm
            assert int(processes) == 100


def test_missing_combinations_matches_the_failed_ones() -> None:
    combinations = _build_test_combinations()
    expected_missing = {c for c in combinations if c.seed == 13}

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        runner.run_all(FAKE_SIMULATOR, combinations, output_dir, process_count=100)

        missing = runner.missing_combinations(combinations, output_dir)
        assert set(missing) == expected_missing


def test_rerun_only_retries_missing_and_does_not_repeat_successes() -> None:
    combinations = _build_test_combinations()
    succeeded_combinations = [c for c in combinations if c.seed != 13]

    with tempfile.TemporaryDirectory() as tmp:
        output_dir = Path(tmp)

        first = runner.run_all(FAKE_SIMULATOR, combinations, output_dir, process_count=100)
        assert len(first.failed) == 4  # 2 cenarios x 2 algoritmos, seed 13

        mtimes_after_first = _raw_mtimes(output_dir, succeeded_combinations)

        # segunda chamada: fake_simulator ainda falha para a seed 13.
        second = runner.run_all(FAKE_SIMULATOR, combinations, output_dir, process_count=100)

        # as combinacoes ja bem-sucedidas nao foram executadas de novo:
        # o arquivo de saida delas nao foi reescrito.
        mtimes_after_second = _raw_mtimes(output_dir, succeeded_combinations)
        assert mtimes_after_second == mtimes_after_first

        # a unica coisa reexecutada foi o que ainda faltava (seed 13).
        assert {f.combination for f in second.failed} == {
            c for c in combinations if c.seed == 13
        }
        assert set(second.succeeded) == set(succeeded_combinations)

        # terceira chamada: simula o binario "consertado" via variavel de
        # ambiente que fake_simulator le para decidir quais seeds falham.
        third = runner.run_all(
            FAKE_SIMULATOR, combinations, output_dir, process_count=100,
            extra_env={"FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS": ""},
        )

        assert third.failed == ()
        assert set(third.succeeded) == set(combinations)

        # de novo, as combinacoes que ja tinham sucesso desde a primeira
        # chamada continuam sem ser reexecutadas.
        mtimes_after_third = _raw_mtimes(output_dir, succeeded_combinations)
        assert mtimes_after_third == mtimes_after_first

        # a seed 13 nao aparece mais no log de falhas, ja que passou a
        # funcionar na terceira chamada.
        failures_path = output_dir / "failures.csv"
        with failures_path.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.DictReader(stream))
        assert rows == []


def test_force_fail_flag_also_produces_a_controlled_failure() -> None:
    # segundo mecanismo de falha controlada citado no enunciado, testado
    # diretamente contra o binario fake (sem passar pelo runner).
    result = subprocess.run(
        [
            sys.executable, str(FAKE_SIMULATOR),
            "--scenario", "balanced", "--algorithm", "fcfs",
            "--seed", "1", "--processes", "100", "--force-fail",
        ],
        capture_output=True, text=True,
    )

    assert result.returncode != 0
    assert result.stdout == ""
    assert "falha simulada" in result.stderr


def main() -> None:
    test_failure_log_has_all_required_fields()
    test_missing_combinations_matches_the_failed_ones()
    test_rerun_only_retries_missing_and_does_not_repeat_successes()
    test_force_fail_flag_also_produces_a_controlled_failure()

    print(
        "OK: log de falhas e reexecucao de combinacoes faltantes "
        "validados contra o binario fake."
    )


if __name__ == "__main__":
    main()
