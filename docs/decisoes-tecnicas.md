# Decisões técnicas da equipe

Este documento registra as convenções compartilhadas que devem ser usadas por todos os módulos do simulador.

## Prioridade

- [x] Valor menor representa maior prioridade
- [ ] Valor maior representa maior prioridade

**Decisão final:** prioridade `0` representa a maior prioridade. Valores maiores representam prioridades progressivamente menores.

## Modelo de tempo

- [x] Tempo discreto
- [ ] Simulação por eventos

**Decisão final:** a simulação será executada em tempo discreto, utilizando ticks inteiros.

## Ordem dos eventos em cada tick

A ordem de processamento será determinística:

1. concluir operações de E/S cujo contador terminou;
2. mover esses processos de `BLOCKED` para `READY`;
3. admitir processos cujo tempo de chegada corresponde ao tick atual (`NEW -> READY`);
4. se a CPU estiver livre, consultar o escalonador para selecionar o próximo processo;
5. aplicar troca de contexto, quando necessária;
6. executar um tick de CPU;
7. ao final do tick, verificar término de rajada, bloqueio por E/S, término do processo ou preempção.

Se durante a implementação aparecer um caso de borda não coberto por essa sequência, a equipe deve preservar a mesma regra para todos os algoritmos e registrar a alteração neste arquivo.

## Representação dos processos

Cada processo será representado por uma sequência alternada de rajadas:

```text
CPU -> E/S -> CPU -> E/S -> CPU
```

O processo sempre começa e termina com uma rajada de CPU.

A estrutura deve preservar os valores originais das rajadas para permitir o cálculo posterior do tempo mínimo ideal e do slowdown.

## Modelo de chegada

Os tempos de chegada serão gerados aleatoriamente a partir da seed e dos parâmetros do cenário.

Os processos não chegarão obrigatoriamente todos no instante `0`.

A mesma seed, no mesmo cenário e com a mesma configuração, deve produzir exatamente os mesmos tempos de chegada e a mesma carga completa.

## E/S

### Quando o processo solicita E/S?

Quando termina uma rajada de CPU e ainda existe uma rajada de E/S na sequência do processo.

A transição será:

```text
RUNNING -> BLOCKED
```

### Por quanto tempo permanece bloqueado?

Pela duração da rajada de E/S correspondente, gerada junto com a carga de trabalho.

### Operações de E/S podem ocorrer em paralelo?

**Sim.** Múltiplos processos podem permanecer em E/S simultaneamente.

### Há dispositivo/fila própria de E/S?

**Não.** No modelo principal não haverá dispositivo único nem fila própria de E/S. Cada processo bloqueado mantém seu próprio contador de E/S.

Essa simplificação deve ser mencionada no artigo como uma escolha de modelagem que pode influenciar os resultados.

### Quando o processo retorna à fila de prontos?

Quando o contador de sua rajada de E/S chega a zero.

A transição será:

```text
BLOCKED -> READY
```

O processo retorna ao final da fila de prontos. A partir daí, a política do escalonador determina quando ele voltará a executar.

## Troca de contexto

### Custo principal

`2 ticks`, configurável.

Os experimentos principais usarão esse mesmo custo para todos os algoritmos.

### Quando ocorre?

Uma troca de contexto é contabilizada quando um processo deixa a CPU e outro processo diferente passa a executar.

Isso inclui, por exemplo:

- término de um processo seguido da execução de outro;
- bloqueio por E/S seguido da execução de outro;
- preempção seguida da execução de outro processo.

Se o mesmo processo continuar executando, não há nova troca.

### CPU fica indisponível durante a troca?

**Sim.** Durante os `2 ticks` da troca, nenhum processo executa rajada de CPU.

### Há custo quando a CPU sai do estado ocioso?

**Não.** A transição de CPU ociosa para um processo não será contabilizada como troca de contexto e não terá custo adicional.

## Round Robin

### Quantum principal

`4 ticks`, configurável.

Experimentos complementares podem testar outros valores, mas o valor usado nos experimentos principais deve ser registrado junto aos resultados.

### Comportamento

Ao expirar o quantum, se o processo ainda estiver executável:

```text
RUNNING -> READY
```

Ele retorna ao final da fila.

Se terminar ou bloquear exatamente antes/ao final de seu quantum, não deve ser criada uma preempção artificial.

## Critérios de desempate gerais

### FCFS

1. ordem de entrada na fila de prontos;
2. menor PID, somente se for necessário resolver um empate impossível de distinguir pela ordem da fila.

### Round Robin

A própria ordem da fila circular determina o próximo processo. Novas chegadas, retornos de E/S e processos preemptados entram no final da fila conforme a ordem em que os eventos forem processados no tick.

### Prioridade não preemptiva

1. menor valor numérico de prioridade;
2. quem entrou primeiro na fila de prontos;
3. menor PID.

A chegada de um processo mais prioritário não interrompe o processo atualmente executando.

### Algoritmo próprio

O critério será definido junto com a política final do algoritmo próprio e registrado nesta seção antes dos experimentos finais.

## Índice de Jain

O índice será calculado internamente na escala:

```text
0.0 a 1.0
```

Para gráficos, tabelas e apresentação, poderá ser convertido para porcentagem:

```text
Jain (%) = Jain * 100
```

Valores próximos de `1.0` / `100%` representam maior igualdade entre os slowdowns.

## Algoritmo próprio

Ainda será fechado na issue específica do algoritmo próprio.

Direção inicial aprovada para investigação: combinar prioridade dinâmica/aging com informações disponíveis no estado atual e no histórico já observado, buscando reduzir starvation sem degradar excessivamente o turnaround.

O algoritmo não poderá acessar a duração exata de rajadas futuras ainda não observadas.

Antes da implementação final, esta seção deverá registrar:

- problema que tenta resolver;
- informações usadas;
- regra de seleção;
- comportamento preemptivo ou não;
- hipótese de melhoria;
- limitações esperadas.

## Regra de consistência

As decisões deste documento devem ser usadas igualmente por FCFS, Round Robin, Prioridade e algoritmo próprio.

Qualquer alteração posterior em modelagem de E/S, troca de contexto, ordem de eventos ou convenções compartilhadas deve ser discutida pela equipe e registrada aqui antes da execução dos experimentos finais.
