# Contribuição

## Fluxo recomendado

Cada integrante trabalha em uma branch própria por tarefa:

```text
feature/david-simulator
feature/carlos-workload
feature/levi-schedulers
feature/henrique-metrics
```

Evite branches permanentes por pessoa; depois da primeira rodada, prefira branches por funcionalidade.

## Commits

Use mensagens claras, por exemplo:

```text
feat(simulator): implementa bloqueio por E/S
feat(workload): adiciona cenário io-bound
test(rr): valida preempção por quantum
fix(metrics): corrige cálculo do slowdown
```

## Contratos compartilhados

Alterações em `include/*.h` podem afetar toda a equipe. Antes de mudar um contrato já usado por outra pessoa:

1. explique a necessidade no grupo;
2. mantenha a mudança pequena;
3. atualize os módulos afetados;
4. faça merge apenas depois de compilar os módulos integrados.

## Regra importante

Todo integrante deve conseguir explicar o código que commita e participar tecnicamente da apresentação.
