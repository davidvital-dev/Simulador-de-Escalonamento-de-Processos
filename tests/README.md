# Testes

Antes de executar cargas com 1.000 processos, validar manualmente cargas pequenas (3 a 20 processos).

Casos mínimos:

- processo com uma única rajada de CPU;
- chegadas em tempos diferentes;
- CPU -> E/S -> CPU;
- dois processos em E/S simultaneamente;
- CPU ociosa;
- custo de troca de contexto;
- FCFS sem preempção;
- Round Robin com quantum pequeno;
- prioridade não preemptiva;
- determinismo da seed;
- métricas calculadas à mão.

## Teste do gerador de cargas

Para executar toda a suíte do gerador:

```bash
make test-workload
```

Esse alvo cobre as 100 seeds nos quatro cenários, estrutura das rajadas, perfis
experimentais, configurações personalizadas, impressão de debug e sanidade da
lista de seeds.

Para verificar o determinismo nos quatro cenários:

```bash
make test-workload-determinism
```

Para verificar PID, chegada, prioridade e sequências de rajadas:

```bash
make test-workload-bursts
```

Para verificar se os quatro perfis experimentais são distintos e coerentes:

```bash
make test-workload-scenarios
```

Para verificar configurações personalizadas e configurações inválidas:

```bash
make test-workload-config
```

Para verificar a impressão de depuração de cargas pequenas:

```bash
make test-workload-debug
```

Para verificar a lista fixa de 100 seeds experimentais:

```bash
make test-experiment-seeds
```

## Testes do motor da simulação

Para executar a suíte do motor:

```bash
make test-simulator
```

Ela é dividida em três grupos.

### Runtime e cópia profunda

```bash
make test-process-runtime
```

Valida reset dos campos mutáveis, restauração dos tempos restantes e cópia
profunda das rajadas.

### Estados, chegadas e E/S

```bash
make test-simulator-states
```

Valida `NEW -> READY -> RUNNING`, `RUNNING -> BLOCKED -> READY`, término,
chegadas em tempos diferentes, CPU ociosa e operações de E/S paralelas.

### Troca de contexto e preempção

```bash
make test-simulator-context
```

Valida o custo principal de 2 ticks, ausência de custo em `idle -> processo` e
o contrato de preempção usado por políticas como Round Robin.

## Suíte completa

```bash
make test
```

Executa os testes do gerador e do motor disponíveis no repositório.
