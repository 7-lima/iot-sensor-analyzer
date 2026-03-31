# Projeto 1 - Analisador de Logs de Sensores IoT
**Aluno:** Rafael de Souza Alves de Lima  
**RA:** 10425819  

## Arquivos da entrega
- `sensor_analyzer_seq.c` -> versão sequencial
- `sensor_analyzer_par.c` -> versão paralela com mutex global
- `sensor_analyzer_optimized.c` -> versão paralela otimizada
- `generate_sensor_log.py` -> gerador de logs
- `Makefile` -> compilação e execução
- `benchmark.py` -> mede tempos, speedup e eficiência e gera gráfico
- `relatorio.pdf` -> relatório em PDF
- `relatorio.docx` -> relatório editável em Word
- `benchmark_out/` -> saídas e gráfico gerados no ambiente de teste

## Requisitos
- Linux
- `gcc`
- `make`
- `python3`
- biblioteca `pthread`
- biblioteca matemática `libm`

## Como compilar
Na pasta do projeto:
```bash
make
```

Isso gera:
```bash
sensor_analyzer_seq
sensor_analyzer_par
sensor_analyzer_optimized
```

## Como gerar logs

### Log rápido para teste
```bash
make gerar_log_teste
```

### Log grande para análise de desempenho
```bash
make gerar_log
```

O alvo `gerar_log` usa:
```bash
python3 generate_sensor_log.py --sensores 120 --dias 1 --output sensores.log
```

Isso gera mais de 10 milhões de linhas, atendendo a exigência do projeto.

## Como executar

### Versão sequencial
```bash
./sensor_analyzer_seq sensores_teste.log
```

### Versão paralela com mutex
```bash
./sensor_analyzer_par 4 sensores_teste.log
```

### Versão paralela otimizada
```bash
./sensor_analyzer_optimized 4 sensores_teste.log
```

## Comandos prontos do Makefile

### Rodar a versão sequencial no log de teste
```bash
make run_seq
```

### Rodar a versão paralela com 1, 2, 4, 8 e 16 threads
```bash
make run_par
```

### Rodar a versão otimizada com 1, 2, 4, 8 e 16 threads
```bash
make run_opt
```

### Rodar benchmark completo e gerar gráfico
```bash
make benchmark
```

Os resultados ficam em:
```bash
benchmark_out/
```

## O que o programa calcula
- média de temperatura por sensor
- sensor mais instável
- total de alertas e críticos
- consumo total de energia
- detecção de anomalias por desvio maior que 3 desvios padrão

## Estratégias implementadas

### 1. Sequencial
Lê o arquivo inteiro em memória e processa linha por linha em uma única thread.

### 2. Paralela com mutex global
Divide o vetor de linhas em blocos contíguos e cada thread atualiza as estruturas globais protegidas por mutex.

### 3. Paralela otimizada
Cada thread mantém acumuladores locais e, no fim, faz uma redução para a estrutura global. Isso reduz muito a contenção.

## Como apresentar para o professor

### Roteiro rápido de fala
1. **Introdução**
   - O projeto analisa logs de sensores IoT de um data center.
   - O objetivo é comparar uma solução sequencial, uma paralela com mutex e uma paralela otimizada.

2. **Estruturas de dados**
   - As linhas do arquivo são carregadas em um vetor de `LinhaLog`.
   - As estatísticas por sensor ficam em um vetor indexado pelo número do sensor.

3. **Versão sequencial**
   - Uma única thread percorre todas as linhas.
   - Atualiza soma, soma dos quadrados, mínimo, máximo, energia e contadores.

4. **Versão paralela com mutex**
   - O vetor é dividido em blocos contíguos.
   - Cada thread processa uma faixa.
   - O mutex protege a seção crítica nas atualizações globais.

5. **Versão otimizada**
   - Cada thread usa acumuladores locais.
   - Não há mutex no caminho principal do processamento.
   - Só existe sincronização na redução final e no vetor de anomalias.

6. **Resultados**
   - Mostrar o gráfico de speedup.
   - Explicar que a versão com mutex perde desempenho por contenção.
   - Explicar que a otimizada escala melhor por reduzir sincronização.

7. **Conclusão**
   - Paralelismo ajuda quando o overhead de sincronização é controlado.
   - A otimização mostra melhor speedup e eficiência.

## Dicas para a apresentação
- Abra o terminal e rode primeiro:
```bash
make run_seq
```
- Depois:
```bash
make run_par
```
- Depois:
```bash
make run_opt
```
- Por fim:
```bash
make benchmark
```
- Mostre o arquivo `benchmark_out/grafico_speedup.png`.
- Se o professor perguntar sobre anomalias, explique que a leitura é anômala quando:
```text
|valor - média| / desvio > 3
```

## Estrutura esperada para entrega em zip
```text
sensor_analyzer_seq.c
sensor_analyzer_par.c
sensor_analyzer_optimized.c
Makefile
generate_sensor_log.py
relatorio.pdf
```
