#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <vector>
#include "lcgrand.cpp"  /* Encabezado para el generador de numeros aleatorios */

using namespace std;

class SistemaMMM{
    struct Servidor{
        int estado;
        float tiempo_sig_salida;
        float tiempo_ocupado;
        int clientes_atendidos;

        Servidor(){
            estado = 0;
            tiempo_sig_salida = 1.0e+29;
            tiempo_ocupado = 0.0;
            clientes_atendidos = 0;
        }
    };

    static int const LIMITE_COLA = 100;  /* Capacidad maxima de la cola */
    static int const OCUPADO = 1; /* Indicador de Servidor Ocupado */
    static int const LIBRE = 0;  /* Indicador de Servidor Libre */
    static int const NUM_SERVIDORES = 3;  /* Número de servidores en el sistema (m) */

    int   sig_tipo_evento, num_clientes_atendidos, num_esperas_requerido, num_eventos, num_servidores,
            num_clientes_cola, estado_servidor[NUM_SERVIDORES + 1], sig_servidor_salida;
    float area_num_entra_cola, area_estado_servidor[NUM_SERVIDORES +1], media_entre_llegadas, media_atencion,
            tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[3],
            total_de_esperas;
    FILE  *parametros, *resultados;

    vector<Servidor> servidores;

public :
    int main(){
        parametros = fopen("MMm/param.txt", "r");
        resultados = fopen("MMm/result.txt", "w");

        num_eventos = 2;

        fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
               &num_esperas_requerido, &num_servidores);

        fprintf(resultados, "Sistema de Colas Simple (M/M/m)\n\n");
        fprintf(resultados, "Tiempo promedio de llegada: %11.3f minutos\n\n", media_entre_llegadas);
        fprintf(resultados, "Tiempo promedio de atenciï¿½n: %16.3f minutos\n\n", media_atencion);
        fprintf(resultados, "Número de clientes: %14d\n\n", num_esperas_requerido);
        fprintf(resultados, "Número de servidores: %14d\n\n", num_servidores);

        inicializar();
        while (num_clientes_atendidos < num_esperas_requerido) {
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
        /*for (int i = 1; i <= num_servidores; ++i) {
            if (estado_servidor[i] == LIBRE) {
                return i;
            }
        }*/

        for (int i = 0; i < servidores.size(); i ++){
            if(servidores[i].estado == LIBRE){
                return i;
            }
        }
        return -1;  // Todos los servidores estï¿½n ocupados
    }

    void inicializar(void) {
        tiempo_simulacion = 0.0;

        for (int i = 0; i <= LIMITE_COLA; ++i) {
            tiempo_llegada[i] = 0.0;
        }

        for (int i = 1; i <= num_servidores; ++i) {
            area_estado_servidor[i] = 0.0;
            estado_servidor[i] = LIBRE;

            servidores.emplace_back();
        }

        num_clientes_cola = 0;
        tiempo_ultimo_evento = 0.0;

        num_clientes_atendidos = 0;
        total_de_esperas = 0.0;
        area_num_entra_cola = 0.0;

        tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        tiempo_sig_evento[2] = 1.0e+30;
    }

    void controltiempo(void) {
        int i;
        float min_tiempo_sig_evento = 1.0e+29;
        float min_tiempo_sig_salida = 1.0e+30;

        sig_tipo_evento = 0;

        // encontrar la siguiente salida y ubicarla en tiempo_sig_evento[2]

        for (i = 0; i < servidores.size(); i++){
            if(servidores[i].tiempo_sig_salida < min_tiempo_sig_salida){
                min_tiempo_sig_salida = servidores[i].tiempo_sig_salida;
                sig_servidor_salida = i;
            }
        }

        tiempo_sig_evento[2] = min_tiempo_sig_salida;

        for (i = 1; i <= num_eventos; ++i)
            if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
                min_tiempo_sig_evento = tiempo_sig_evento[i];
                sig_tipo_evento = i;
            }

        if (sig_tipo_evento == 0) {
            fprintf(resultados, "\nLa lista de eventos estï¿½ vacï¿½a %f", tiempo_simulacion);
            exit(1);
        }

        tiempo_simulacion = min_tiempo_sig_evento;
    }

    void llegada(void) {
        float espera;

        // Programa la siguiente llegada
        tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

        int servidor_libre = encontrar_servidor_libre();

        if (servidor_libre != -1) {
            espera = 0.0;
            total_de_esperas += espera;

            ++num_clientes_atendidos;

            //Programa siguiente salida

            servidores[servidor_libre].tiempo_sig_salida = tiempo_simulacion + expon(media_atencion);

            servidores[servidor_libre].estado = OCUPADO;

        } else {
            if (num_clientes_cola < LIMITE_COLA) {
                ++num_clientes_cola;

                if (num_clientes_cola > LIMITE_COLA) {
                    fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
                    fprintf(resultados, "%f", tiempo_simulacion);
                    exit(2);
                }

                tiempo_llegada[num_clientes_cola] = tiempo_simulacion;
            }
        }
    }

    void salida(void) {
        int i;
        float espera;

        servidores[sig_servidor_salida].estado = LIBRE;

        if (num_clientes_cola == 0) {
            for (int i = 0; i <= servidores.size(); ++i) {
                servidores[i].estado = LIBRE;
                servidores[i].tiempo_sig_salida = 1.0e+30;
            }
        } else {
            --num_clientes_cola;

            espera = tiempo_simulacion - tiempo_llegada[1];
            total_de_esperas += espera;

            ++num_clientes_atendidos;

            int servidor_libre = encontrar_servidor_libre();

            printf("servidor libre: %d\n",servidor_libre);

            if (servidor_libre != -1) {
                servidores[servidor_libre].tiempo_sig_salida = tiempo_simulacion + expon(media_atencion);
                servidores[servidor_libre].estado = OCUPADO;


                //Mover clientes en la fila
                for (i = 1; i <= num_clientes_cola; ++i) {
                    tiempo_llegada[i] = tiempo_llegada[i + 1];
                }

            } else {
                tiempo_sig_evento[2] = 1.0e+30;
            }
        }
    }

    void reportes(void) {
        fprintf(resultados, "\n\nEspera promedio en la cola: %11.3f minutos\n\n",
                total_de_esperas / num_clientes_atendidos);
        fprintf(resultados, "Nï¿½mero promedio en cola: %10.3f\n\n",
                area_num_entra_cola / tiempo_simulacion);

        for (int i = 0; i <= num_servidores; ++i) {
            fprintf(resultados, "Uso del servidor %d: %15.3f\n\n",
                    i + 1, area_estado_servidor[i] / tiempo_simulacion);
        }

        fprintf(resultados, "Tiempo de terminaciï¿½n de la simulaciï¿½n: %12.3f minutos", tiempo_simulacion);
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
        area_num_entra_cola      += num_clientes_cola * time_since_last_event;

        /* Actualiza el ï¿½rea bajo la funciï¿½n indicadora de servidor ocupado para mï¿½ltiples servidores. */
        for (int i = 1; i <= num_servidores; ++i) {
            area_estado_servidor[i] += estado_servidor[i] * time_since_last_event;
        }
    }


    float expon(float media)  /* Funcion generadora de la exponencias */
    {
        /* Retorna una variable aleatoria exponencial con media "media"*/

        return -media * log(lcgrand(1));
    }
};

int main(){
    SistemaMMM mmm;
    mmm.main();
}