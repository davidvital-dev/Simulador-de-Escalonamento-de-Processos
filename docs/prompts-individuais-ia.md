# Prompts individuais para as IAs da equipe

Cada integrante deve copiar apenas a sua seção e enviar para a IA que estiver usando. O repositório é a fonte de verdade: antes de alterar contratos compartilhados em `include/*.h`, a IA deve inspecionar o código existente e propor a menor mudança possível.

Todos devem produzir código e testes. Ninguém fica apenas com documentação.

---

## David — motor da simulação

Você está me ajudando no projeto **Simulador de Escalonamento de Processos**, em C. Meu nome é David e sou responsável pelo motor da simulação, estados dos processos, E/S, troca de contexto e integração.

Antes de programar, inspecione o repositório e identifique as structs e APIs já existentes. Não crie um segundo simulador nem duplique lógica dos escalonadores.

### Entregas

1. Completar a representação de processo, incluindo rajadas e campos necessários à simulação.
2. Implementar cópia profunda das cargas para que todos os algoritmos recebam a mesma carga inicial.
3. Implementar tempo discreto, chegadas `NEW -> READY`, despacho `READY -> RUNNING`, execução de CPU, bloqueio por E/S, retorno `BLOCKED -> READY`, término e CPU ociosa.
4. Implementar custo de troca de contexto configurável e idêntico para todos os algoritmos.
5. Oferecer suporte à preempção sem colocar política de Round Robin dentro de `simulator.c`.
6. Criar logs opcionais de debug para cargas pequenas.
7. Integrar parâmetros principais do executável.
8. Criar testes para CPU simples, `CPU -> E/S -> CPU`, E/S simultânea, CPU ociosa, troca de contexto e deep copy.

Explique cada decisão, mostre os arquivos alterados, como compilar e como testar. Não faça grandes refatorações fora do escopo.

---

## Carlos — geração, seeds e cenários

Você está me ajudando no projeto **Simulador de Escalonamento de Processos**, em C. Meu nome é Carlos e sou responsável pela geração determinística das cargas, seeds e quatro cenários experimentais.

Antes de programar, leia `Process`, a API do motor e a forma como a carga é armazenada. Não altere headers compartilhados sem necessidade.

### Entregas

1. Criar uma configuração centralizada de cenário (`ScenarioConfig` ou equivalente).
2. Criar gerador que receba seed, cenário e quantidade de processos.
3. Garantir que mesma seed + cenário + configuração gere exatamente a mesma carga.
4. Gerar PID, chegada, prioridade e sequências válidas `CPU -> E/S -> ... -> CPU`.
5. Implementar os cenários equilibrado, I/O-bound, CPU-bound e prioridades desbalanceadas.
6. Deixar todos os intervalos numéricos configuráveis e claramente identificados como decisões da equipe, não exigências do enunciado.
7. Criar impressão/debug de cargas pequenas.
8. Criar pelo menos 100 seeds reproduzíveis para os experimentos.
9. Criar testes de determinismo e sanidade dos quatro cenários.

Não gere uma carga diferente dentro de cada algoritmo. Gere uma vez e deixe o motor clonar para FCFS, RR, Prioridade e algoritmo próprio.

---

## Levi — escalonadores

Você está me ajudando no projeto **Simulador de Escalonamento de Processos**, em C. Meu nome é Levi e sou responsável por FCFS, Round Robin, Prioridade não preemptiva e pela implementação principal do algoritmo próprio.

O motor compartilhado controla tempo, CPU, E/S, estados e troca de contexto. Meu módulo deve decidir **quem executar** e, quando aplicável, **quando preemptar**.

### Entregas

1. Implementar FCFS sem preempção e com desempate determinístico.
2. Implementar Round Robin com quantum configurável, mantendo corretamente a ordem da fila após preempções.
3. Implementar Prioridade não preemptiva; uma chegada de maior prioridade não interrompe o processo atual.
4. Criar testes manuais e linhas do tempo esperadas para os três algoritmos.
5. Junto com a equipe, definir o algoritmo próprio: problema, informações usadas, regra de seleção, preempção, hipótese de melhoria e limitações.
6. Implementar `proposed.c` sem usar conhecimento futuro de rajadas ainda não observadas.
7. Testar o algoritmo próprio em starvation, prioridades desbalanceadas e mistura de processos curtos/longos.

Uma direção possível para o algoritmo próprio é combinar prioridade base, aging e histórico já observado, mas isso é apenas uma sugestão da equipe e precisa ser validado antes da implementação final.

---

## Henrique — métricas e experimentos

Você está me ajudando no projeto **Simulador de Escalonamento de Processos**. Meu nome é Henrique e sou responsável por métricas em C, CSV, automação dos experimentos, estatística e gráficos.

### Entregas

1. Calcular turnaround por processo e turnaround médio.
2. Calcular tempo mínimo ideal a partir das rajadas originais de CPU e E/S.
3. Calcular slowdown e índice de Jain aplicado aos slowdowns.
4. Integrar o contador oficial de trocas de contexto do motor.
5. Criar estrutura de resultado e exportação CSV com uma linha por execução.
6. Validar as métricas à mão em uma carga de 3 a 5 processos.
7. Criar script para executar automaticamente cenário × seed × algoritmo.
8. Rodar os experimentos principais: 4 cenários × pelo menos 100 seeds × 4 algoritmos.
9. Detectar execuções faltantes, NaN, infinito e valores inválidos; nunca descartar seeds silenciosamente.
10. Consolidar média e IC95%, usando `média ± 1,96 * s / sqrt(n)` com desvio padrão amostral.
11. Gerar gráficos para turnaround, trocas de contexto e Jain, sempre mostrando média e IC95%.

Explique também como interpretar cada métrica e os trade-offs entre desempenho e justiça.

---

# Checkpoint coletivo do algoritmo próprio

Os quatro precisam conseguir responder:

1. Qual problema o algoritmo tenta resolver?
2. Quais informações ele usa?
3. Como escolhe o próximo processo?
4. Ele é preemptivo?
5. Por que esperamos melhora?
6. Qual custo introduz?
7. Em quais cenários esperamos piora?
8. Ele usa somente estado atual e histórico observado?

# Checklist antes de aceitar código produzido por IA

- Eu consigo explicar o código?
- Ele respeita a arquitetura real do repositório?
- Ele duplicou funcionalidade de outro integrante?
- Compila?
- Há testes reproduzíveis?
- E/S e troca de contexto seguem a mesma modelagem em todos os algoritmos?
- Valores importantes são configuráveis?
- Memória alocada é liberada?
- O algoritmo próprio evita conhecimento futuro?
- Mudanças de headers foram alinhadas com a equipe?
