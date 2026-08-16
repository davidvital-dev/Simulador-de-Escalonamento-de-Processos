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

O algoritmo próprio preserva a ordem da fila quando dois processos obtêm o mesmo score. Como a fila é montada deterministicamente pelo motor, isso também torna o desempate determinístico; o PID já é usado pelo motor para ordenar eventos simultâneos quando necessário.

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

## Algoritmo próprio: AHR

O algoritmo próprio da equipe será chamado de **AHR - Aging com Histórico de Rajadas**. No código e no pipeline experimental ele continua identificado como `proposed` para manter uma interface simples com os demais algoritmos.

### Problema que tenta resolver

O AHR busca reduzir o risco de starvation causado por prioridade estática sem abandonar completamente a prioridade original. Ao mesmo tempo, usa apenas o histórico de CPU já observado para favorecer processos que demonstraram comportamento de rajadas curtas, tentando evitar uma degradação excessiva do turnaround.

### Informações usadas em cada decisão

Para cada processo em `READY`, o algoritmo pode usar somente informações disponíveis naquele instante:

- prioridade base do processo;
- instante em que entrou pela última vez em `READY` (`ready_since`);
- tempo atual da simulação;
- total de CPU já executado pelo processo;
- quantidade de rajadas de CPU já concluídas.

O algoritmo **não lê** a duração da rajada de CPU atual nem qualquer rajada futura, não consulta `Burst.duration` e não consulta `remaining_time`. A interface `SchedulerProcessView` usada pelo escalonador não expõe essas informações.

### Estimativa baseada no histórico

Quando o processo já concluiu pelo menos uma rajada de CPU, sua estimativa histórica é a média inteira das rajadas observadas:

```text
historico = total_cpu_executed / completed_cpu_bursts
```

Para processos que ainda não concluíram nenhuma rajada, usa-se uma estimativa inicial fixa e configurável. O valor padrão é `8 ticks`. Essa estimativa é um parâmetro do algoritmo e não depende da rajada real futura.

### Aging

O tempo de espera atual é:

```text
espera = max(0, tempo_atual - ready_since)
```

A cada `aging_interval` ticks de espera, o processo recebe um ponto de bônus no score. O intervalo padrão é `4 ticks`.

### Regra de seleção

Para cada processo pronto, calcula-se:

```text
score = priority_weight * prioridade
      + burst_weight * historico
      - floor(espera / aging_interval)
```

Parâmetros padrão:

```text
priority_weight = 8
burst_weight = 1
aging_interval = 4
initial_burst_estimate = 8
```

Esses quatro valores ficam **congelados antes dos experimentos principais** e serão os mesmos nos quatro cenários e em todas as seeds. Eles não serão ajustados cenário a cenário depois de observar os resultados. Experimentos complementares de sensibilidade podem testar outros valores, desde que sejam apresentados separadamente e não substituam a configuração principal.

O processo com **menor score** é escolhido.

Interpretação dos parâmetros padrão:

- uma prioridade numericamente pior aumenta o score e torna o processo menos favorecido inicialmente;
- um histórico de rajadas menores reduz a desvantagem relativa no turnaround;
- o termo de aging cresce sem limite com o tempo de espera e, portanto, progressivamente compensa prioridade baixa e histórico de rajadas longas.

Em empate de score, vence quem já está antes na fila de prontos.

### Preempção

O AHR é **não preemptivo**. A decisão é refeita quando a CPU fica disponível após término, bloqueio ou ociosidade, mas a chegada de um processo com score melhor não interrompe quem já está executando.

Essa escolha evita criar trocas de contexto adicionais apenas por mudanças de score e permite comparar o efeito da regra de seleção sem misturá-lo com uma política agressiva de preempção.

### Hipótese de melhoria

A hipótese principal é que o aging aumente a igualdade entre slowdowns em relação à Prioridade não preemptiva, melhorando o índice de Jain principalmente no cenário de prioridades desbalanceadas. O componente de histórico tenta preservar ou melhorar turnaround ao favorecer processos que já demonstraram rajadas curtas de CPU.

Como o algoritmo é não preemptivo, espera-se que o número de trocas de contexto permaneça mais próximo de FCFS/Prioridade do que de Round Robin.

Essas são hipóteses; os experimentos finais determinarão se elas realmente aparecem nas métricas.

### Inspiração e modificação

O AHR combina duas ideias clássicas de escalonamento:

- **aging**, usado para aumentar progressivamente a chance de processos que aguardam por muito tempo;
- **favorecimento de rajadas curtas**, inspirado na motivação de políticas como SJF/SRTF e em técnicas de previsão de rajadas.

A modificação da equipe é não usar conhecimento da próxima rajada. Em vez disso, o AHR usa exclusivamente a média das rajadas de CPU que o processo já concluiu e mistura essa informação com prioridade base e aging em um único score configurável.

### Limitações esperadas

- a estimativa inicial fixa pode favorecer ou prejudicar processos novos dependendo do cenário;
- a média de todo o histórico reage lentamente se o comportamento do processo mudar de fase;
- os pesos são parâmetros empíricos e podem influenciar fortemente o resultado;
- por ser não preemptivo, o algoritmo não consegue interromper uma rajada longa que já começou;
- o AHR reduz o risco de starvation apenas nos pontos em que a CPU volta a ficar disponível;
- favorecer histórico curto pode prejudicar processos CPU-bound, embora o aging tenda a compensar essa desvantagem com o tempo.

## Regra de consistência

As decisões deste documento devem ser usadas igualmente por FCFS, Round Robin, Prioridade e algoritmo próprio.

Qualquer alteração posterior em modelagem de E/S, troca de contexto, ordem de eventos ou convenções compartilhadas deve ser discutida pela equipe e registrada aqui antes da execução dos experimentos finais.
