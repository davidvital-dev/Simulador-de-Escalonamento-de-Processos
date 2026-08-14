#!/usr/bin/env python3
"""Binário fake do simulador, usado só para testar o pipeline.

Imita a interface de linha de comando que o `main.c` real vai expor
(`--scenario`, `--seed`, `--algorithm`, `--processes`) sem depender do
motor real nem dos escalonadores, que ainda não existem. Serve para
`scripts/run_experiments.py` ser escrito e testado contra um binário
externo de verdade (via subprocess), pronto para trocar de alvo quando o
binário real aceitar os mesmos argumentos -- nenhuma lógica do runner
precisa mudar, só o caminho passado em `--binary`.

Convenção de falha controlada, para testar detecção de erros no runner:
- a(s) seed(s) em `FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS` (variável de
  ambiente, lista separada por vírgula; default: `13`) sempre falham;
- a flag `--force-fail` força falha independente da seed.
Em ambos os casos, sai com código != 0 e uma mensagem em stderr, sem
imprimir nada em stdout.
"""

from __future__ import annotations

import argparse
import os
import sys

ALWAYS_FAIL_SEEDS_ENV = "FAKE_SIMULATOR_ALWAYS_FAIL_SEEDS"
DEFAULT_ALWAYS_FAIL_SEEDS = frozenset({13})


def always_fail_seeds() -> frozenset[int]:
    raw = os.environ.get(ALWAYS_FAIL_SEEDS_ENV)
    if raw is None:
        return DEFAULT_ALWAYS_FAIL_SEEDS
    raw = raw.strip()
    if not raw:
        return frozenset()
    return frozenset(int(part) for part in raw.split(","))


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--seed", required=True, type=int)
    parser.add_argument("--algorithm", required=True)
    parser.add_argument("--processes", required=True, type=int)
    parser.add_argument(
        "--force-fail", action="store_true",
        help="forca falha (exit code != 0) independente da seed",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if args.force_fail or args.seed in always_fail_seeds():
        print(
            "fake_simulator: falha simulada "
            f"(seed={args.seed}, scenario={args.scenario}, "
            f"algorithm={args.algorithm}, force_fail={args.force_fail})",
            file=sys.stderr,
        )
        return 1

    # Valores ficticios, so para o runner ter uma linha de CSV real para
    # capturar -- nao correspondem a uma simulacao de verdade.
    turnaround_medio = 100.0 + args.seed
    trocas_contexto = 50 + args.seed
    jain_slowdown = 0.9

    print(
        f"{args.seed},{args.scenario},{args.algorithm},{args.processes},"
        f"{turnaround_medio:.2f},{trocas_contexto},{jain_slowdown:.4f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
