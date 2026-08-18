#!/usr/bin/env python3
"""Teste ponta a ponta da interface de linha de comando do simulador real."""

from __future__ import annotations

import csv
import subprocess
import sys
from pathlib import Path


if len(sys.argv) != 2:
    raise SystemExit(f"uso: {Path(sys.argv[0]).name} CAMINHO_DO_SIMULADOR")

BINARY = Path(sys.argv[1]).resolve()
SCENARIOS = ("balanced", "io-bound", "cpu-bound", "priority-imbalanced")
ALGORITHMS = ("fcfs", "rr", "priority", "proposed")


def run_simulator(*arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(BINARY), *arguments],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def valid_arguments(
    scenario: str = "balanced",
    algorithm: str = "fcfs",
    seed: str = "7",
    processes: str = "20",
) -> list[str]:
    return [
        "--scenario", scenario,
        "--algorithm", algorithm,
        "--seed", seed,
        "--processes", processes,
    ]


def parse_result(result: subprocess.CompletedProcess[str]) -> list[str]:
    assert result.returncode == 0, result.stderr
    assert result.stderr == "", result.stderr

    lines = result.stdout.splitlines()
    assert len(lines) == 1, result.stdout
    row = next(csv.reader(lines))
    assert len(row) == 7, row
    return row


def test_help() -> None:
    result = run_simulator("--help")
    assert result.returncode == 0
    assert result.stderr == ""
    assert "--scenario" in result.stdout
    assert "--algorithm" in result.stdout
    assert "--context-switch-cost" in result.stdout


def test_all_scenarios_and_algorithms() -> None:
    for scenario in SCENARIOS:
        for algorithm in ALGORITHMS:
            row = parse_result(
                run_simulator(*valid_arguments(scenario, algorithm))
            )
            assert row[:4] == ["7", scenario, algorithm, "20"]
            assert float(row[4]) >= 0.0
            assert int(row[5]) >= 0
            assert 0.0 < float(row[6]) <= 1.0


def test_same_input_is_deterministic() -> None:
    arguments = valid_arguments("io-bound", "proposed", "42", "30")
    first = run_simulator(*arguments)
    second = run_simulator(*arguments)
    assert first.returncode == second.returncode == 0
    assert first.stdout == second.stdout
    assert first.stderr == second.stderr == ""


def test_configurable_parameters() -> None:
    result = run_simulator(
        *valid_arguments(algorithm="rr"),
        "--quantum", "2",
        "--context-switch-cost", "0",
    )
    row = parse_result(result)
    assert row[2] == "rr"


def test_debug_goes_only_to_stderr() -> None:
    result = run_simulator(*valid_arguments(processes="3"), "--debug")
    assert result.returncode == 0
    assert len(result.stdout.splitlines()) == 1
    assert "READY -> RUNNING" in result.stderr


def test_invalid_arguments_fail_without_csv() -> None:
    invalid_cases = (
        [],
        valid_arguments(scenario="unknown"),
        valid_arguments(algorithm="unknown"),
        valid_arguments(seed="-1"),
        valid_arguments(processes="0"),
        [*valid_arguments(algorithm="rr"), "--quantum", "0"],
        [*valid_arguments(), "--context-switch-cost", "-1"],
        [*valid_arguments(), "--unknown"],
    )

    for arguments in invalid_cases:
        result = run_simulator(*arguments)
        assert result.returncode != 0, arguments
        assert result.stdout == "", (arguments, result.stdout)
        assert result.stderr != "", arguments


def main() -> None:
    assert BINARY.is_file(), BINARY
    test_help()
    test_all_scenarios_and_algorithms()
    test_same_input_is_deterministic()
    test_configurable_parameters()
    test_debug_goes_only_to_stderr()
    test_invalid_arguments_fail_without_csv()
    print("OK: CLI real validada nos quatro cenarios e quatro algoritmos.")


if __name__ == "__main__":
    main()
