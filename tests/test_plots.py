#!/usr/bin/env python3
"""Valida os gráficos comparativos com uma tabela consolidada sintética.

Usa `stats.summarize_table` sobre dados fictícios para gerar uma tabela
consolidada controlada e confirma que
`plots.build_metric_figure`/`plots.plot_all_metrics`: (1) produzem título,
eixos e legenda com o conteúdo esperado, e (2) gravam os 3 arquivos de
imagem obrigatórios sem erro. Também grava os 3 PNGs em results/ (o
diretório de saída já usado pelo projeto) para conferência visual manual.
"""

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import matplotlib.pyplot as plt  # noqa: E402  (import depende do sys.path acima)
import plots  # noqa: E402
import stats  # noqa: E402

RESULTS_DIR = Path(__file__).resolve().parents[1] / "results"

# Carga ficticia: 2 seeds por cenario+algoritmo, cobrindo os 4 cenarios e
# os 4 algoritmos obrigatorios, no formato de colunas do CSV exportado
# pelo simulador em C.
_SCENARIO_BASELINES = [
    # cenario, turnaround base, trocas de contexto base, jain base
    ("balanced", 120.0, 500, 0.90),
    ("io-bound", 200.0, 900, 0.85),
    ("cpu-bound", 90.0, 300, 0.93),
    ("priority-imbalanced", 150.0, 600, 0.70),
]
_ALGORITHM_JAIN_SHIFTS = [
    ("fcfs", 0.0), ("rr", 0.02), ("priority", -0.05), ("proposed", 0.03),
]
_SEEDS = (1, 2)


def _build_synthetic_rows() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for scenario, base_turnaround, base_switches, base_jain in _SCENARIO_BASELINES:
        for algorithm, jain_shift in _ALGORITHM_JAIN_SHIFTS:
            for seed in _SEEDS:
                rows.append({
                    "seed": seed,
                    "cenario": scenario,
                    "algoritmo": algorithm,
                    "n_processos": 1000,
                    "turnaround_medio": base_turnaround + seed,
                    "trocas_contexto": base_switches + seed * 10,
                    "jain_slowdown": min(0.99, base_jain + jain_shift + seed * 0.01),
                })
    return rows


SYNTHETIC_ROWS = _build_synthetic_rows()


def build_synthetic_table() -> list[dict[str, object]]:
    table = stats.summarize_table(SYNTHETIC_ROWS)
    assert len(table) == 4 * 4 * 3, "esperado 4 cenarios x 4 algoritmos x 3 metricas"
    return table


def test_figure_has_title_axes_legend_and_unit() -> None:
    table = build_synthetic_table()

    figure = plots.build_metric_figure(table, "turnaround_medio")
    axis = figure.axes[0]

    title = axis.get_title()
    assert "Turnaround médio" in title
    assert "95" in title, "titulo deve indicar que a barra de erro e IC95%"
    assert "n=2 sementes" in title

    assert axis.get_xlabel() == "Cenário"
    scenario_labels = [label.get_text() for label in axis.get_xticklabels()]
    assert scenario_labels == [
        plots.SCENARIO_LABELS[scenario] for scenario in plots.SCENARIO_ORDER
    ]

    ylabel = axis.get_ylabel()
    assert "Turnaround médio" in ylabel
    assert "ticks" in ylabel, "eixo deve indicar a unidade (ticks)"

    _, legend_labels = axis.get_legend_handles_labels()
    assert legend_labels == [
        plots.ALGORITHM_LABELS[algorithm] for algorithm in plots.ALGORITHM_ORDER
    ]

    tallest_bar = max(patch.get_height() for patch in axis.patches)
    assert axis.get_ylim()[1] >= tallest_bar * 1.1, (
        "o topo deve reservar espaco para IC95% e rotulos"
    )

    plt.close(figure)


def test_all_three_mandatory_metrics_have_units_or_range() -> None:
    table = build_synthetic_table()

    for metric, expected_fragment in [
        ("turnaround_medio", "ticks"),
        ("trocas_contexto", "contagem"),
        ("jain_slowdown", "%"),
    ]:
        figure = plots.build_metric_figure(table, metric)
        axis = figure.axes[0]
        assert expected_fragment in axis.get_ylabel()
        plt.close(figure)


def test_jain_is_presented_as_percentage_without_changing_input_table() -> None:
    table = build_synthetic_table()
    original_means = [
        row["media"] for row in table if row["metrica"] == "jain_slowdown"
    ]

    figure = plots.build_metric_figure(table, "jain_slowdown")
    axis = figure.axes[0]
    bar_heights = [patch.get_height() for patch in axis.patches]

    assert max(bar_heights) > 90, "Jain deve ser exibido na escala percentual"
    assert max(bar_heights) <= 100
    assert axis.get_ylim() == (0.0, 105.0)
    assert original_means == [
        row["media"] for row in table if row["metrica"] == "jain_slowdown"
    ], "a conversao visual nao deve alterar os dados consolidados"

    plt.close(figure)


def test_plot_all_metrics_writes_three_valid_png_files() -> None:
    table = build_synthetic_table()
    png_magic = b"\x89PNG\r\n\x1a\n"

    with tempfile.TemporaryDirectory() as tmp:
        paths = plots.plot_all_metrics(table, tmp)

        assert len(paths) == 3
        expected_names = {
            "turnaround_medio.png", "trocas_contexto.png", "jain_slowdown.png"
        }
        assert {path.name for path in paths} == expected_names

        for path in paths:
            assert path.is_file()
            data = path.read_bytes()
            assert data[:8] == png_magic, f"{path} nao comeca com a assinatura PNG"
            assert len(data) > 1024, f"{path} parece vazio/corrompido demais"


def write_reference_images_for_manual_inspection() -> list[Path]:
    table = build_synthetic_table()
    return plots.plot_all_metrics(table, RESULTS_DIR)


def main() -> None:
    test_figure_has_title_axes_legend_and_unit()
    test_all_three_mandatory_metrics_have_units_or_range()
    test_jain_is_presented_as_percentage_without_changing_input_table()
    test_plot_all_metrics_writes_three_valid_png_files()

    generated = write_reference_images_for_manual_inspection()
    for path in generated:
        print(f"grafico gerado: {path}")

    print(
        "OK: titulo, eixos, unidade, legenda e os 3 arquivos PNG "
        "(turnaround, trocas de contexto, Jain) validados."
    )


if __name__ == "__main__":
    main()
