
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_SENSORES      1000
#define MAX_ID_LEN        16
#define MAX_TIPO_LEN      16
#define MAX_STATUS_LEN    10
#define MAX_TIMESTAMP_LEN 20
#define BUFFER_SIZE       256
#define MAX_ANOMALIAS     50000
#define MAX_THREADS       64

typedef struct {
    char   id[MAX_ID_LEN];
    char   tipo[MAX_TIPO_LEN];
    long   contagem;
    double soma;
    double soma_quadrados;
    double min_valor;
    double max_valor;
    int    ativo;
} EstatisticaSensor;

typedef struct {
    char   sensor_id[MAX_ID_LEN];
    char   timestamp[MAX_TIMESTAMP_LEN * 2];
    char   tipo[MAX_TIPO_LEN];
    double valor;
    double media;
    double desvio;
} Anomalia;

typedef struct {
    char   sensor_id[MAX_ID_LEN];
    char   timestamp[MAX_TIMESTAMP_LEN * 2];
    char   tipo[MAX_TIPO_LEN];
    float  valor;
    char   status[MAX_STATUS_LEN];
} LinhaLog;

static EstatisticaSensor sensores_global[MAX_SENSORES];
static Anomalia          anomalias[MAX_ANOMALIAS];
static long              num_anomalias  = 0;
static long              total_alertas  = 0;
static long              total_criticos = 0;
static double            energia_total  = 0.0;

static LinhaLog *linhas_log = NULL;
static long      num_linhas = 0;

static inline void copiar_str(char *dest, size_t dest_sz, const char *src) {
    if (dest_sz == 0) return;
    snprintf(dest, dest_sz, "%s", src ? src : "");
}

static void inicializar_estatisticas(EstatisticaSensor *arr, int n) {
    memset(arr, 0, sizeof(EstatisticaSensor) * n);
    for (int i = 0; i < n; i++) {
        arr[i].min_valor =  1e18;
        arr[i].max_valor = -1e18;
    }
}

static int id_para_indice(const char *sensor_id) {
    const char *p = strrchr(sensor_id, '_');
    if (!p) return -1;
    int idx = atoi(p + 1);
    if (idx < 1 || idx >= MAX_SENSORES) return -1;
    return idx;
}

static double calcular_media_sensor(const EstatisticaSensor *s) {
    if (!s || s->contagem == 0) return 0.0;
    return s->soma / (double)s->contagem;
}

static double calcular_desvio_sensor(const EstatisticaSensor *s) {
    if (!s || s->contagem < 2) return 0.0;
    double media = calcular_media_sensor(s);
    double var = (s->soma_quadrados - (double)s->contagem * media * media) / (double)(s->contagem - 1);
    if (var < 0.0) var = 0.0;
    return sqrt(var);
}

static long carregar_arquivo(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    long count = 0;
    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), fp)) {
        count++;
    }
    rewind(fp);

    linhas_log = (LinhaLog *)malloc((size_t)count * sizeof(LinhaLog));
    if (!linhas_log) {
        fclose(fp);
        fprintf(stderr, "Erro: memoria insuficiente para %ld linhas.\n", count);
        return -1;
    }

    long idx = 0;
    char data[12], hora[10], palavra_status[10];
    while (fgets(buf, sizeof(buf), fp) && idx < count) {
        LinhaLog *l = &linhas_log[idx];
        int campos = sscanf(buf, "%15s %11s %9s %15s %f %9s %9s",
                            l->sensor_id, data, hora, l->tipo, &l->valor,
                            palavra_status, l->status);
        if (campos == 7 && strcmp(palavra_status, "status") == 0) {
            snprintf(l->timestamp, sizeof(l->timestamp), "%s %s", data, hora);
            idx++;
        }
    }

    fclose(fp);
    return idx;
}

static void __attribute__((unused)) detectar_anomalias_sequencial(void) {
    num_anomalias = 0;
    for (long i = 0; i < num_linhas; i++) {
        LinhaLog *l = &linhas_log[i];
        int idx = id_para_indice(l->sensor_id);
        if (idx < 0 || !sensores_global[idx].ativo) continue;

        double media  = calcular_media_sensor(&sensores_global[idx]);
        double desvio = calcular_desvio_sensor(&sensores_global[idx]);

        if (desvio > 0.0 && fabs((double)l->valor - media) / desvio > 3.0) {
            if (num_anomalias < MAX_ANOMALIAS) {
                Anomalia *an = &anomalias[num_anomalias++];
                copiar_str(an->sensor_id, sizeof(an->sensor_id), l->sensor_id);
                copiar_str(an->timestamp, sizeof(an->timestamp), l->timestamp);
                copiar_str(an->tipo, sizeof(an->tipo), l->tipo);
                an->valor = l->valor;
                an->media = media;
                an->desvio = desvio;
            }
        }
    }
}

static void exibir_resultados(const char *titulo, const char *estrategia, int num_threads, double tempo_exec) {
    printf("\n============================================================\n");
    if (num_threads > 0) {
        printf("  %s | %d thread(s)\n", titulo, num_threads);
    } else {
        printf("  %s\n", titulo);
    }
    printf("  %s\n", estrategia);
    printf("============================================================\n\n");

    printf("Total de linhas processadas : %ld\n", num_linhas);
    printf("Tempo de execucao           : %.6f segundos\n\n", tempo_exec);

    printf("------------------------------------------------------------\n");
    printf("MEDIAS DE TEMPERATURA POR SENSOR (primeiros 10)\n");
    printf("------------------------------------------------------------\n");
    printf("%-15s %10s %12s %12s %12s\n", "Sensor", "Leituras", "Media", "Min", "Max");

    int exibidos = 0;
    for (int i = 1; i < MAX_SENSORES && exibidos < 10; i++) {
        if (sensores_global[i].ativo && strcmp(sensores_global[i].tipo, "temperatura") == 0) {
            printf("%-15s %10ld %12.2f %12.2f %12.2f\n",
                   sensores_global[i].id,
                   sensores_global[i].contagem,
                   calcular_media_sensor(&sensores_global[i]),
                   sensores_global[i].min_valor,
                   sensores_global[i].max_valor);
            exibidos++;
        }
    }

    double maior_desvio = -1.0;
    int sensor_instavel = -1;
    for (int i = 1; i < MAX_SENSORES; i++) {
        if (sensores_global[i].ativo && strcmp(sensores_global[i].tipo, "temperatura") == 0) {
            double desvio = calcular_desvio_sensor(&sensores_global[i]);
            if (desvio > maior_desvio) {
                maior_desvio = desvio;
                sensor_instavel = i;
            }
        }
    }

    printf("\n------------------------------------------------------------\n");
    printf("SENSOR MAIS INSTAVEL\n");
    printf("------------------------------------------------------------\n");
    if (sensor_instavel >= 0) {
        printf("ID: %s | Desvio padrao: %.6f\n",
               sensores_global[sensor_instavel].id, maior_desvio);
    } else {
        printf("Nenhum sensor de temperatura encontrado.\n");
    }

    printf("\n------------------------------------------------------------\n");
    printf("TOTAIS\n");
    printf("------------------------------------------------------------\n");
    printf("Total de ALERTA + CRITICO : %ld\n", total_alertas + total_criticos);
    printf("  ALERTA                  : %ld\n", total_alertas);
    printf("  CRITICO                 : %ld\n", total_criticos);
    printf("Consumo total de energia  : %.2f Wh\n", energia_total);

    printf("\n------------------------------------------------------------\n");
    printf("ANOMALIAS (ate 10 primeiras de %ld)\n", num_anomalias);
    printf("------------------------------------------------------------\n");
    long limite = num_anomalias < 10 ? num_anomalias : 10;
    for (long i = 0; i < limite; i++) {
        printf("%-12s | %-19s | %-12s | valor=%8.2f | media=%8.2f | desvio=%8.2f\n",
               anomalias[i].sensor_id,
               anomalias[i].timestamp,
               anomalias[i].tipo,
               anomalias[i].valor,
               anomalias[i].media,
               anomalias[i].desvio);
    }
    if (num_anomalias == 0) {
        printf("Nenhuma anomalia encontrada.\n");
    }
}

typedef struct {
    EstatisticaSensor sensores[MAX_SENSORES];
    long              total_alertas;
    long              total_criticos;
    double            energia_total;
} AcumuladorLocal;

static pthread_mutex_t mutex_reducao  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_anomalia = PTHREAD_MUTEX_INITIALIZER;
static AcumuladorLocal *acumuladores  = NULL;

typedef struct {
    int  thread_id;
    long inicio;
    long fim;
} ArgThread;

static void *thread_acumular_local(void *arg) {
    ArgThread *a = (ArgThread *)arg;
    AcumuladorLocal *ac = &acumuladores[a->thread_id];

    memset(ac, 0, sizeof(AcumuladorLocal));
    for (int i = 0; i < MAX_SENSORES; i++) {
        ac->sensores[i].min_valor =  1e18;
        ac->sensores[i].max_valor = -1e18;
    }

    for (long i = a->inicio; i < a->fim; i++) {
        LinhaLog *l = &linhas_log[i];
        int idx = id_para_indice(l->sensor_id);
        if (idx < 0) continue;

        EstatisticaSensor *s = &ac->sensores[idx];
        if (!s->ativo) {
            copiar_str(s->id, sizeof(s->id), l->sensor_id);
            copiar_str(s->tipo, sizeof(s->tipo), l->tipo);
            s->ativo = 1;
        }

        s->contagem++;
        s->soma += l->valor;
        s->soma_quadrados += (double)l->valor * l->valor;
        if (l->valor < s->min_valor) s->min_valor = l->valor;
        if (l->valor > s->max_valor) s->max_valor = l->valor;

        if (strcmp(l->status, "ALERTA") == 0) ac->total_alertas++;
        else if (strcmp(l->status, "CRITICO") == 0) ac->total_criticos++;

        if (strcmp(l->tipo, "energia") == 0) ac->energia_total += l->valor;
    }

    pthread_mutex_lock(&mutex_reducao);

    total_alertas += ac->total_alertas;
    total_criticos += ac->total_criticos;
    energia_total += ac->energia_total;

    for (int i = 1; i < MAX_SENSORES; i++) {
        if (!ac->sensores[i].ativo) continue;

        EstatisticaSensor *gs = &sensores_global[i];
        EstatisticaSensor *ls = &ac->sensores[i];

        if (!gs->ativo) {
            copiar_str(gs->id, sizeof(gs->id), ls->id);
            copiar_str(gs->tipo, sizeof(gs->tipo), ls->tipo);
            gs->ativo = 1;
        }

        gs->contagem += ls->contagem;
        gs->soma += ls->soma;
        gs->soma_quadrados += ls->soma_quadrados;
        if (ls->min_valor < gs->min_valor) gs->min_valor = ls->min_valor;
        if (ls->max_valor > gs->max_valor) gs->max_valor = ls->max_valor;
    }

    pthread_mutex_unlock(&mutex_reducao);
    return NULL;
}

static void *thread_detectar_anomalias(void *arg) {
    ArgThread *a = (ArgThread *)arg;

    for (long i = a->inicio; i < a->fim; i++) {
        LinhaLog *l = &linhas_log[i];
        int idx = id_para_indice(l->sensor_id);
        if (idx < 0 || !sensores_global[idx].ativo) continue;

        double media  = calcular_media_sensor(&sensores_global[idx]);
        double desvio = calcular_desvio_sensor(&sensores_global[idx]);

        if (desvio > 0.0 && fabs((double)l->valor - media) / desvio > 3.0) {
            pthread_mutex_lock(&mutex_anomalia);
            if (num_anomalias < MAX_ANOMALIAS) {
                Anomalia *an = &anomalias[num_anomalias++];
                copiar_str(an->sensor_id, sizeof(an->sensor_id), l->sensor_id);
                copiar_str(an->timestamp, sizeof(an->timestamp), l->timestamp);
                copiar_str(an->tipo, sizeof(an->tipo), l->tipo);
                an->valor = l->valor;
                an->media = media;
                an->desvio = desvio;
            }
            pthread_mutex_unlock(&mutex_anomalia);
        }
    }
    return NULL;
}

static void executar_paralelo(int num_threads, void *(*func)(void *)) {
    pthread_t threads[MAX_THREADS];
    ArgThread args[MAX_THREADS];
    long bloco = num_linhas / num_threads;

    for (int t = 0; t < num_threads; t++) {
        args[t].thread_id = t;
        args[t].inicio = t * bloco;
        args[t].fim = (t == num_threads - 1) ? num_linhas : (t + 1) * bloco;
        if (pthread_create(&threads[t], NULL, func, &args[t]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <num_threads> <arquivo_de_log>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads < 1 || num_threads > MAX_THREADS) {
        fprintf(stderr, "Numero de threads deve estar entre 1 e %d.\n", MAX_THREADS);
        return 1;
    }

    inicializar_estatisticas(sensores_global, MAX_SENSORES);

    printf("Carregando arquivo: %s\n", argv[2]);
    num_linhas = carregar_arquivo(argv[2]);
    if (num_linhas < 0) return 1;
    printf("Linhas carregadas: %ld | Threads: %d\n", num_linhas, num_threads);

    acumuladores = (AcumuladorLocal *)malloc((size_t)num_threads * sizeof(AcumuladorLocal));
    if (!acumuladores) {
        fprintf(stderr, "Erro: memoria insuficiente para acumuladores locais.\n");
        free(linhas_log);
        return 1;
    }

    struct timespec t_inicio, t_fim;
    clock_gettime(CLOCK_MONOTONIC, &t_inicio);

    executar_paralelo(num_threads, thread_acumular_local);
    executar_paralelo(num_threads, thread_detectar_anomalias);

    clock_gettime(CLOCK_MONOTONIC, &t_fim);
    double tempo = (t_fim.tv_sec - t_inicio.tv_sec) + (t_fim.tv_nsec - t_inicio.tv_nsec) / 1e9;

    exibir_resultados("RESULTADOS - ANALISADOR PARALELO OTIMIZADO",
                      "Estrategia: acumuladores locais por thread + reducao final",
                      num_threads,
                      tempo);

    free(acumuladores);
    free(linhas_log);
    return 0;
}
