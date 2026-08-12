# Cenários de carga

Os quatro tipos de cenário fazem parte do enunciado do projeto. Os valores
numéricos desta página, porém, são **decisões experimentais da equipe**, não
exigências do enunciado. Eles devem ser registrados junto aos resultados e
podem ser ajustados antes dos experimentos finais.

Todos os intervalos são inclusivos e ficam centralizados em `ScenarioConfig`,
declarada em `include/workload.h`. O gerador não depende diretamente dos
valores-padrão: uma configuração pode ser copiada, alterada e validada antes de
ser passada para `workload_generate`.

## Parâmetros-padrão

| Cenário | Intervalo entre chegadas | Prioridade | Rajadas de CPU | Duração de CPU | Duração de E/S | Viés de prioridade |
|---|---:|---:|---:|---:|---:|---|
| Equilibrado | 0–3 | 0–9 | 1–5 | 4–12 | 4–12 | uniforme |
| I/O-bound | 0–3 | 0–9 | 2–6 | 1–4 | 10–30 | uniforme |
| CPU-bound | 0–3 | 0–9 | 1–3 | 15–40 | 1–5 | uniforme |
| Prioridades desbalanceadas | 0–3 | 0–9 | 1–5 | 4–12 | 4–12 | 80% em 0–1 |

O primeiro processo chega no tick `0`. Para os demais, o intervalo sorteado é
somado ao tempo de chegada anterior. Uma carga com `N` rajadas de CPU contém
`N - 1` rajadas de E/S, preservando a sequência `CPU -> E/S -> ... -> CPU`.

## Campos configuráveis

- `interarrival_time`: intervalo entre chegadas consecutivas;
- `priority`: faixa completa de prioridades, sendo `0` a maior;
- `cpu_burst_count`: quantidade de rajadas de CPU por processo;
- `cpu_burst_duration`: duração de cada rajada de CPU;
- `io_burst_duration`: duração de cada rajada de E/S;
- `priority_bias_percent`: probabilidade de usar a faixa com viés;
- `biased_priority`: faixa usada quando o viés é aplicado.

Antes de gerar uma carga personalizada, a configuração deve ser verificada com
`workload_config_is_valid`.
