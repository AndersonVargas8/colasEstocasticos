/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp"  /* Encabezado para el generador de numeros aleatorios */

#define LIMITE_COLA 100  /* Capacidad maxima de la cola */
#define OCUPADO      1  /* Indicador de Servidor Ocupado */
#define LIBRE      0  /* Indicador de Servidor Libre */
#define NUM_SERVIDORES 100  /* N�mero de servidores en el sistema (m) */

using namespace std;

int   sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos, num_servidores,
      num_entra_cola, estado_servidor[NUM_SERVIDORES];
float area_num_entra_cola, area_estado_servidor[NUM_SERVIDORES], media_entre_llegadas, media_atencion,
      tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[3],
      total_de_esperas;
FILE  *parametros, *resultados;

void  inicializar(void);
void  controltiempo(void);
void  llegada(void);
void  salida(void);
void  reportes(void);
void  actualizar_estad_prom_tiempo(void);
float expon(float mean);
int encontrar_servidor_libre(void);

int main(void) {
    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");

    num_eventos = 2;

    fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
           &num_esperas_requerido, &num_servidores);

    fprintf(resultados, "Sistema de Colas Simple (M/M/m)\n\n");
    fprintf(resultados, "Tiempo promedio de llegada: %11.3f minutos\n\n", media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atenci�n: %16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "N�mero de clientes: %14d\n\n", num_esperas_requerido);
    fprintf(resultados, "N�mero de servidores: %14d\n\n", num_servidores);

    inicializar();

    while (num_clientes_espera < num_esperas_requerido) {
        controltiempo();
        actualizar_estad_prom_tiempo();

        switch (sig_tipo_evento) {
            case 1:
                llegada();
                break;
            case 2:
                salida();
                break;
        }
    }

    reportes();

    fclose(parametros);
    fclose(resultados);

    return 0;
}

int encontrar_servidor_libre(void) {
    for (int i = 0; i < num_servidores; ++i) {
        if (estado_servidor[i] == LIBRE) {
            return i;
        }
    }
    return -1;  // Todos los servidores est�n ocupados
}

void inicializar(void) {
    tiempo_simulacion = 0.0;

    for (int i = 0; i <= LIMITE_COLA; ++i) {
        tiempo_llegada[i] = 0.0;
    }

    for (int i = 0; i < num_servidores; ++i) {
        area_estado_servidor[i] = 0.0;
        estado_servidor[i] = LIBRE;
    }

    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;
}

void controltiempo(void) {
    int i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }

    if (sig_tipo_evento == 0) {
        fprintf(resultados, "\nLa lista de eventos est� vac�a %f", tiempo_simulacion);
        exit(1);
    }

    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void) {
    float espera;

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

    int servidor_libre = encontrar_servidor_libre();

    if (servidor_libre != -1) {
        espera = 0.0;
        total_de_esperas += espera;

        ++num_clientes_espera;
        tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
        estado_servidor[servidor_libre] = OCUPADO;
    } else {
        if (num_entra_cola < LIMITE_COLA) {
            ++num_entra_cola;

            if (num_entra_cola > LIMITE_COLA) {
                fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
                fprintf(resultados, "%f", tiempo_simulacion);
                exit(2);
            }

            tiempo_llegada[num_entra_cola] = tiempo_simulacion;
        }
    }
}

void salida(void) {
    int i;
    float espera;

    if (num_entra_cola == 0) {
        for (int i = 0; i < num_servidores; ++i) {
            estado_servidor[i] = LIBRE;
        }
        tiempo_sig_evento[2] = 1.0e+30;
    } else {
        --num_entra_cola;

        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        ++num_clientes_espera;

        int servidor_libre = encontrar_servidor_libre();

        printf("servidor libre: %d\n",servidor_libre);

        if (servidor_libre != -1) {
            tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);

            for (i = 1; i <= num_entra_cola; ++i) {
                tiempo_llegada[i] = tiempo_llegada[i + 1];
            }

            estado_servidor[servidor_libre] = OCUPADO;
        } else {
            tiempo_sig_evento[2] = 1.0e+30;
        }
    }
}

void reportes(void) {
    fprintf(resultados, "\n\nEspera promedio en la cola: %11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "N�mero promedio en cola: %10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);

    for (int i = 0; i < num_servidores; ++i) {
        fprintf(resultados, "Uso del servidor %d: %15.3f\n\n",
                i + 1, area_estado_servidor[i] / tiempo_simulacion);
    }

    fprintf(resultados, "Tiempo de terminaci�n de la simulaci�n: %12.3f minutos", tiempo_simulacion);
}

void actualizar_estad_prom_tiempo(void)  /* Actualiza los acumuladores de
														area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
    	del ultimo evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento       = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola      += num_entra_cola * time_since_last_event;

    /* Actualiza el �rea bajo la funci�n indicadora de servidor ocupado para m�ltiples servidores. */
    for (int i = 0; i < num_servidores; ++i) {
        area_estado_servidor[i] += estado_servidor[i] * time_since_last_event;
    }
}


float expon(float media)  /* Funcion generadora de la exponencias */
{
    /* Retorna una variable aleatoria exponencial con media "media"*/

    return -media * log(lcgrand(1));
}

