#!/usr/bin/env python3
"""Gráficos comparativos das métricas agregadas por cenário e algoritmo.

Módulo isolado, sem I/O de CSV nem orquestração de experimentos: recebe a
tabela consolidada que `scripts.stats.summarize_table` produz e gera um
gráfico de barras por métrica, comparando os quatro
algoritmos obrigatórios dentro de cada um dos quatro cenários obrigatórios,
com barra de erro representando o IC95%.

Paleta categórica (fcfs, rr, priority, proposed) validada com a skill de
dataviz do projeto para o par adjacente usado em gráficos de barra: veja
`ALGORITHM_COLORS`.
"""

from __future__ import annotations

from pathlib import Path
from typing import Mapping, Sequence

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt  # noqa: E402  (backend precisa ser fixado antes)
from matplotlib.figure import Figure  # noqa: E402

SCENARIO_ORDER = ("balanced", "io-bound", "cpu-bound", "priority-imbalanced")
ALGORITHM_ORDER = ("fcfs", "rr", "priority", "proposed")

SCENARIO_LABELS = {
    "balanced": "Equilibrado",
    "io-bound": "Orientado a I/O",
    "cpu-bound": "Orientado a CPU",
    "priority-imbalanced": "Prioridades\ndesbalanceadas",
}

ALGORITHM_LABELS = {
    "fcfs": "FCFS",
    "rr": "Round Robin",
    "priority": "Prioridade",
    "proposed": "AHR (proposto)",
}

# Slots 1-4 da paleta categórica validada (blue, orange, aqua, yellow),
# em ordem fixa por algoritmo -- nunca atribuída por rank ou ciclada.
ALGORITHM_COLORS = {
    "fcfs": "#2a78d6",
    "rr": "#eb6834",
    "priority": "#1baf7a",
    "proposed": "#eda100",
}

CHART_SURFACE = "#fcfcfb"
GRID_COLOR = "#e1e0d9"
INK_PRIMARY = "#0b0b0b"
INK_SECONDARY = "#52514e"

METRIC_INFO = {
    "turnaround_medio": {
        "label": "Turnaround médio",
        "unit": "ticks",
        "scale": 1.0,
    },
    "trocas_contexto": {
        "label": "Trocas de contexto",
        "unit": "contagem",
        "scale": 1.0,
    },
    "jain_slowdown": {
        "label": "Índice de Jain do slowdown",
        "unit": "%",
        "scale": 100.0,
    },
}


def _format_value(metric: str, value: float) -> str:
    """Formata rótulos compactos, legíveis e coerentes com o eixo."""
    if metric == "jain_slowdown":
        return f"{value:.1f}%".replace(".", ",")
    if abs(value) >= 1000:
        return f"{value / 1000:.1f} mil".replace(".", ",")
    if metric == "trocas_contexto":
        return f"{value:.0f}"
    return f"{value:.1f}".replace(".", ",")


def build_metric_figure(
    table: Sequence[Mapping[str, object]],
    metric: str,
    *,
    scenario_order: Sequence[str] = SCENARIO_ORDER,
    algorithm_order: Sequence[str] = ALGORITHM_ORDER,
) -> Figure:
    """Monta (sem salvar) o gráfico de barras de uma métrica.

    `table` segue o formato de `scripts.stats.summarize_table`: uma linha
    por (cenario, algoritmo, metrica), com "media", "ic95_inferior" e
    "ic95_superior". Só as linhas com `metrica == metric` são usadas.

    A barra de erro usa o IC95% relatado na tabela (ic95_superior menos
    media para o lado de cima, media menos ic95_inferior para o lado de
    baixo), em vez de recalcular -- funciona com qualquer tabela nesse
    formato, mesmo que o intervalo não seja simétrico em torno da média.

    Combinações cenário+algoritmo ausentes em `table` ficam com barra e
    erro zero, sem rótulo de valor, para manter os quatro grupos e as
    quatro barras visíveis mesmo com uma tabela parcial (útil para os
    dados sintéticos atuais), em vez de reindexar o gráfico em silêncio.
    """
    if metric not in METRIC_INFO:
        raise ValueError(f"metrica desconhecida: {metric!r}")

    info = METRIC_INFO[metric]
    rows_by_key = {
        (row["cenario"], row["algoritmo"]): row
        for row in table
        if row["metrica"] == metric
    }

    figure, axis = plt.subplots(figsize=(10, 5.6), facecolor=CHART_SURFACE)
    axis.set_facecolor(CHART_SURFACE)

    sample_sizes = {
        int(row["n"])
        for row in table
        if row["metrica"] == metric and row.get("n") is not None
    }
    if len(sample_sizes) == 1:
        sample_note = f"n={sample_sizes.pop()} sementes por combinação"
    else:
        sample_note = "n varia por combinação"

    group_positions = range(len(scenario_order))
    group_width = 0.8
    bar_width = group_width / len(algorithm_order)

    for algorithm_index, algorithm in enumerate(algorithm_order):
        means: list[float] = []
        lower_errors: list[float] = []
        upper_errors: list[float] = []
        labels: list[str] = []

        for scenario in scenario_order:
            row = rows_by_key.get((scenario, algorithm))
            if row is None:
                means.append(0.0)
                lower_errors.append(0.0)
                upper_errors.append(0.0)
                labels.append("")
                continue

            scale = float(info["scale"])
            mean = float(row["media"]) * scale
            means.append(mean)
            lower_errors.append(
                max(mean - float(row["ic95_inferior"]) * scale, 0.0)
            )
            upper_errors.append(
                max(float(row["ic95_superior"]) * scale - mean, 0.0)
            )
            labels.append(_format_value(metric, mean))

        offsets = [
            position - group_width / 2 + bar_width * (algorithm_index + 0.5)
            for position in group_positions
        ]
        bars = axis.bar(
            offsets,
            means,
            width=bar_width * 0.85,
            yerr=[lower_errors, upper_errors],
            capsize=5,
            label=ALGORITHM_LABELS.get(algorithm, algorithm),
            color=ALGORITHM_COLORS.get(algorithm, INK_SECONDARY),
            edgecolor=INK_PRIMARY,
            linewidth=0.6,
            error_kw={
                "elinewidth": 1.4,
                "capthick": 1.4,
                "ecolor": INK_PRIMARY,
            },
            zorder=3,
        )
        # Rotulo de valor visivel sobre cada barra: mitiga o contraste
        # baixo das cores "aqua"/"yellow" contra a superficie clara
        # (regra de alivio da paleta validada) e facilita a leitura exata.
        # Alterna a altura dos rótulos entre séries adjacentes. Isso evita
        # sobreposição quando algoritmos têm médias iguais ou muito próximas.
        label_padding = 3 + (algorithm_index % 2) * 10
        axis.bar_label(bars, labels=labels, padding=label_padding, fontsize=7.5,
                       color=INK_SECONDARY)

    axis.set_xticks(list(group_positions))
    axis.set_xticklabels([
        SCENARIO_LABELS.get(scenario, scenario) for scenario in scenario_order
    ])
    axis.set_xlabel("Cenário")
    axis.set_ylabel(f"{info['label']} ({info['unit']})")
    axis.set_title(
        f"{info['label']} por algoritmo e cenário\n"
        f"Média e IC95% · {sample_note}"
    )
    # Fora da area de plotagem, a direita: posicao fixa que nunca sobrepoe
    # barras, independente da faixa de valores de cada metrica (evita
    # loc="best" escolher um canto ja ocupado por dados).
    axis.legend(title="Algoritmo", frameon=False,
               loc="upper left", bbox_to_anchor=(1.01, 1.0))

    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)
    axis.spines["left"].set_color(GRID_COLOR)
    axis.spines["bottom"].set_color(GRID_COLOR)
    axis.tick_params(colors=INK_SECONDARY)
    axis.grid(axis="y", color=GRID_COLOR, linewidth=0.8, zorder=0)
    axis.set_axisbelow(True)
    axis.set_ylim(bottom=0)
    if metric == "jain_slowdown":
        axis.set_ylim(0, 105)
    else:
        # Reserva espaço acima das barras para rótulos e IC95%, sem invadir
        # o subtítulo quando uma série domina a escala (caso típico do RR).
        axis.set_ylim(0, axis.dataLim.ymax * 1.15 if axis.dataLim.ymax else 1)

    figure.tight_layout()
    return figure


def plot_metric_comparison(
    table: Sequence[Mapping[str, object]],
    metric: str,
    output_path: str | Path,
    *,
    scenario_order: Sequence[str] = SCENARIO_ORDER,
    algorithm_order: Sequence[str] = ALGORITHM_ORDER,
) -> Path:
    """Constrói o gráfico de `metric` (via `build_metric_figure`) e salva.

    O formato do arquivo é decidido pela extensão de `output_path` (por
    exemplo, `.png`). Retorna o `Path` do arquivo gerado.
    """
    figure = build_metric_figure(
        table, metric,
        scenario_order=scenario_order, algorithm_order=algorithm_order,
    )

    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    # bbox_inches="tight" expande a area salva para incluir a legenda, que
    # fica fora dos eixos (veja build_metric_figure).
    figure.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(figure)

    return output_path


def plot_all_metrics(
    table: Sequence[Mapping[str, object]],
    output_dir: str | Path,
    *,
    scenario_order: Sequence[str] = SCENARIO_ORDER,
    algorithm_order: Sequence[str] = ALGORITHM_ORDER,
) -> list[Path]:
    """Gera um PNG por métrica obrigatória (mesma função, sem duplicação).

    Retorna os caminhos gerados, na ordem de `METRIC_INFO`:
    turnaround_medio, trocas_contexto, jain_slowdown.
    """
    output_dir = Path(output_dir)
    return [
        plot_metric_comparison(
            table, metric, output_dir / f"{metric}.png",
            scenario_order=scenario_order, algorithm_order=algorithm_order,
        )
        for metric in METRIC_INFO
    ]
