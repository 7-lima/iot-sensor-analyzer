# Projeto 1 - Analisador de Logs de Sensores IoT
# Rafael de Souza Alves de Lima - RA: 10425819

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lpthread -lm

TARGETS = sensor_analyzer_seq sensor_analyzer_par sensor_analyzer_optimized

LOG_GRANDE = sensores.log
LOG_TESTE  = sensores_teste.log

.PHONY: all clean gerar_log gerar_log_teste run_seq run_par run_opt benchmark

all: $(TARGETS)

sensor_analyzer_seq: sensor_analyzer_seq.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sensor_analyzer_par: sensor_analyzer_par.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sensor_analyzer_optimized: sensor_analyzer_optimized.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(LOG_GRANDE): generate_sensor_log.py
	python3 generate_sensor_log.py --sensores 120 --dias 1 --output $(LOG_GRANDE)

$(LOG_TESTE): generate_sensor_log.py
	python3 generate_sensor_log.py --sensores 10 --dias 1 --output $(LOG_TESTE)

gerar_log: $(LOG_GRANDE)

gerar_log_teste: $(LOG_TESTE)

run_seq: sensor_analyzer_seq $(LOG_TESTE)
	./sensor_analyzer_seq $(LOG_TESTE)

run_par: sensor_analyzer_par $(LOG_TESTE)
	@echo "=== 1 thread ===";  ./sensor_analyzer_par 1  $(LOG_TESTE)
	@echo "=== 2 threads ==="; ./sensor_analyzer_par 2  $(LOG_TESTE)
	@echo "=== 4 threads ==="; ./sensor_analyzer_par 4  $(LOG_TESTE)
	@echo "=== 8 threads ==="; ./sensor_analyzer_par 8  $(LOG_TESTE)
	@echo "=== 16 threads ==="; ./sensor_analyzer_par 16 $(LOG_TESTE)

run_opt: sensor_analyzer_optimized $(LOG_TESTE)
	@echo "=== 1 thread ===";  ./sensor_analyzer_optimized 1  $(LOG_TESTE)
	@echo "=== 2 threads ==="; ./sensor_analyzer_optimized 2  $(LOG_TESTE)
	@echo "=== 4 threads ==="; ./sensor_analyzer_optimized 4  $(LOG_TESTE)
	@echo "=== 8 threads ==="; ./sensor_analyzer_optimized 8  $(LOG_TESTE)
	@echo "=== 16 threads ==="; ./sensor_analyzer_optimized 16 $(LOG_TESTE)

benchmark: all $(LOG_TESTE)
	python3 benchmark.py --log $(LOG_TESTE) --output_dir benchmark_out

clean:
	rm -f $(TARGETS) $(LOG_GRANDE) $(LOG_TESTE)
	rm -rf benchmark_out
