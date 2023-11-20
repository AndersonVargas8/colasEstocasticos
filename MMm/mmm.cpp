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
        float tiempo_ultima_salida;

        Servidor(){
            estado = 0;
            tiempo_sig_salida = 1.0e+30;
            tiempo_ocupado = 0.0;
            clientes_atendidos = 0;
            tiempo_ultima_salida = 0.0;
        }
    };

    static int const OCUPADO = 1; /* Indicador de Servidor Ocupado */
    static int const LIBRE = 0;  /* Indicador de Servidor Libre */

    int   sig_tipo_evento, num_clientes_atendidos, num_esperas_requerido, num_eventos, num_servidores,
            num_clientes_cola, sig_servidor_salida, total_clientes_arribados, total_clientes_encolados,
            clientes_sin_encolar;
    float area_num_entra_cola, media_entre_llegadas, media_atencion,
            tiempo_simulacion, tiempo_ultimo_evento, tiempo_sig_evento[3],
            total_de_esperas;
    FILE  *parametros, *resultados;

    vector<Servidor> servidores;
    vector<float> tiempo_llegada;

public :
    int main(){
        parametros = fopen("MMm/param.txt", "r");
        resultados = fopen("MMm/result.txt", "w");

        num_eventos = 2;

        fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
               &num_esperas_requerido, &num_servidores);

        fprintf(resultados, "Sistema de Colas Simple (M/M/m)\n\n");
        fprintf(resultados, "Tiempo promedio de llegada: %11.3f minutos\n\n", media_entre_llegadas);
        fprintf(resultados, "Tiempo promedio de atención: %16.3f minutos\n\n", media_atencion);
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
        total_clientes_arribados = 0;
        total_clientes_encolados = 0;
        clientes_sin_encolar = 0;
        tiempo_simulacion = 0.0;

        /*for (int i = 0; i <= tiempo_llegada.size() + 1; ++i) {
            tiempo_llegada[i] = 0.0;
        }*/


        for (int i = 1; i <= num_servidores; ++i) {
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
        float min_tiempo_sig_salida = 1.0e+29;

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
            fprintf(resultados, "\nLa lista de eventos está vacía %f", tiempo_simulacion);
            exit(1);
        }

        tiempo_simulacion = min_tiempo_sig_evento;
    }

    void llegada(void) {
        float espera;

        // Programa la siguiente llegada
        if(total_clientes_arribados < num_esperas_requerido)
            tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        else{
            tiempo_sig_evento[1] = 1.0e+29;
            return;
        }

        int servidor_libre = encontrar_servidor_libre();

        if (servidor_libre != -1) {
            clientes_sin_encolar++;
            total_clientes_arribados++;

            ++num_clientes_atendidos;

            //Programa siguiente salida

            servidores[servidor_libre].tiempo_sig_salida = tiempo_simulacion + expon(media_atencion);

            servidores[servidor_libre].estado = OCUPADO;

        } else {
            total_clientes_arribados++;
            num_clientes_cola++;
            total_clientes_encolados++;

            tiempo_llegada.insert(tiempo_llegada.begin(),tiempo_simulacion);

        }
    }

    void salida(void) {
        int i;
        float espera;

        servidores[sig_servidor_salida].estado = LIBRE;
        servidores[sig_servidor_salida].clientes_atendidos++;

        if (num_clientes_cola == 0) {
            for (int i = 0; i <= servidores.size(); ++i) {
                servidores[i].estado = LIBRE;
                servidores[i].tiempo_sig_salida = 1.0e+30;
            }
        } else {
            //Mover clientes en la fila
            /*for (i = 1; i < tiempo_llegada.size() - 1; ++i) {
                tiempo_llegada[i] = tiempo_llegada[i + 1];
            }*/
            float tiempo_llegada_n = tiempo_llegada.back();
            tiempo_llegada.pop_back();

            --num_clientes_cola;

            espera = tiempo_simulacion - tiempo_llegada_n;
            total_de_esperas += espera;

            ++num_clientes_atendidos;

            int servidor_libre = encontrar_servidor_libre();

            printf("servidor libre: %d\n",servidor_libre);


            servidores[servidor_libre].tiempo_sig_salida = tiempo_simulacion + expon(media_atencion);
            servidores[servidor_libre].estado = OCUPADO;

        }
    }

    void reportes(void) {
        cout<<endl<<"total clientes encolados: "<<total_clientes_encolados<<endl;
        cout<<endl<<"total clientes no encolados: "<<clientes_sin_encolar<<endl;
        float promedio_esperas;

        if(total_clientes_encolados == 0)
            promedio_esperas = 0.0;
        else
            promedio_esperas = total_de_esperas / (float)total_clientes_encolados;

        fprintf(resultados, "\n\nEspera promedio en la cola: %11.3f minutos\n\n", promedio_esperas);

        fprintf(resultados, "Número promedio en cola: %10.3f personas por minuto\n\n",
                area_num_entra_cola / tiempo_simulacion);

        for (int i = 0; i < servidores.size(); ++i) {
            fprintf(resultados, "Uso del servidor %d: %15.3f%\n\n",
                    i + 1, servidores[i].tiempo_ocupado / tiempo_simulacion *100);
        }

        fprintf(resultados, "Tiempo de terminación de la simulación: %12.3f minutos", tiempo_simulacion);
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
        area_num_entra_cola      +=  (float)num_clientes_cola * time_since_last_event;

        cout<<"Clientes en cola: "<<num_clientes_cola<<endl;
        cout<<"siguiente salida: "<<tiempo_sig_evento[2]<<endl;
        cout<<"tiempo simulacion: "<<tiempo_simulacion<<endl;
        cout<<"Clientes atendidos: "<<num_clientes_atendidos<<endl<<endl;
        /* Actualiza el área bajo la función indicadora de servidor ocupado para múltiples servidores. */
        for (int i = 0; i < servidores.size(); ++i) {
            servidores[i].tiempo_ocupado += (float)servidores[i].estado * time_since_last_event;
        }
    }


    static float expon(float media)  /* Funcion generadora de la exponencias */
    {
        /* Retorna una variable aleatoria exponencial con media "media"*/

        return (float)(-media * log(lcgrand(1)));
    }
};

int main(){

    SistemaMMM mmm;
    try{
        mmm.main();
    } catch(exception e){
        cout<<"Error, pero seguimos "<<e.what()<<endl;
    }
}