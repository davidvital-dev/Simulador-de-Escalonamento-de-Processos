#!/usr/bin/env python3
"""Valida a lista versionada de seeds dos experimentos principais."""

from pathlib import Path


SEEDS_PATH = Path(__file__).resolve().parents[1] / "config" / "experiment-seeds.txt"
EXPECTED_SEEDS = list(range(1, 101))
UINT64_MAX = (1 << 64) - 1


def load_seeds(path: Path) -> list[int]:
    lines = path.read_text(encoding="utf-8").splitlines()
    assert lines, "a lista de seeds esta vazia"
    assert all(line.strip() == line and line for line in lines), (
        "cada linha deve conter somente uma seed"
    )
    return [int(line, 10) for line in lines]


def main() -> None:
    seeds = load_seeds(SEEDS_PATH)

    assert seeds == EXPECTED_SEEDS, "a lista deve conter exatamente as seeds 1 a 100"
    assert len(seeds) >= 100
    assert len(seeds) == len(set(seeds)), "seeds duplicadas"
    assert all(0 <= seed <= UINT64_MAX for seed in seeds), "seed fora de uint64_t"

    print("OK: 100 seeds experimentais fixas e unicas validadas.")


if __name__ == "__main__":
    main()
