/*
    INTEGRANTES DEL EQUIPO:
    Hermosillo Prado Carlos
    Montes de Oca del Risco Rafael Alejandro
    Ramirez Andrade Uriel Ismael Guadalupe

    PRACTICA 1.4: SINCRONIZACION CLIENTE-SERVIDOR EN MPI CON OPENMP
    - Nodo 0 = Servidor: unico dueno del saldo. Recibe peticiones una por una
      (MPI_Recv con MPI_ANY_SOURCE), asi nunca hay condicion de carrera.
    - Nodos 1..N-1 = Clientes: con OpenMP llenan un arreglo dinamico de
      transacciones y las envian al servidor.
*/

#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace std;

// Tags de los mensajes
#define TAG_TRANSACCION 1
#define TAG_FIN         2
#define TAG_RESPUESTA   3

struct Transaccion {
    int id_cliente;        // Rank del proceso MPI cliente
    int id_hilo_openmp;    // Hilo de OpenMP que origino la solicitud
    int tipo_operacion;    // 1: Deposito, 2: Retiro, 3: Consulta, 0: Finalizar
    double monto;          // Monto a depositar o retirar
};

struct RespuestaServidor {
    bool aprobada;           // true si la transaccion fue exitosa
    double saldo_resultante; // Saldo actualizado tras la operacion
    char mensaje[100];       // Mensaje descriptivo del servidor
};

ofstream archivoLog;   // Log local de cada nodo
string equipo;         // Nombre de la PC (MPI_Get_processor_name)

// Escribe una linea en el log y, si se pide, tambien en pantalla
void registrar(string texto, bool pantalla) {
    archivoLog << texto << endl;
    if (pantalla) cout << texto << endl;
}

const char* nombreOp(int tipo) {
    if (tipo == 1) return "Deposito";
    if (tipo == 2) return "Retiro";
    return "Consulta";
}

void imprimirNombres() {
    cout << "Hermosillo Prado Carlos, Montes de Oca del Risco Rafael Alejandro, Ramirez Andrade Uriel Ismael Guadalupe" << endl;
}

// ============================ SERVIDOR (NODO 0) ============================
void servidor(int size, int numOps, double& saldo, bool detalle) {
    int clientesActivos = size - 1;
    int total = 0;
    char linea[300];

    // Historial con memoria dinamica (sin vector ni arreglo estatico)
    Transaccion* historial = new Transaccion[(size - 1) * numOps];

    while (clientesActivos > 0) {
        Transaccion t;
        MPI_Status estado;
        // Se atiende UNA peticion a la vez, de cualquier cliente
        MPI_Recv(&t, sizeof(Transaccion), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);

        RespuestaServidor r;
        r.aprobada = true;

        if (estado.MPI_TAG == TAG_FIN) {   // un cliente termino
            clientesActivos--;
            r.saldo_resultante = saldo;
            snprintf(r.mensaje, 100, "Fin recibido");
            MPI_Send(&r, sizeof(RespuestaServidor), MPI_BYTE, estado.MPI_SOURCE, TAG_RESPUESTA, MPI_COMM_WORLD);
            continue;
        }

        if (t.tipo_operacion == 1) {                 // Deposito
            saldo += t.monto;
            snprintf(r.mensaje, 100, "Deposito aprobado");
        }
        else if (t.tipo_operacion == 2) {          // Retiro
            if (saldo >= t.monto) {
                saldo -= t.monto;
                snprintf(r.mensaje, 100, "Retiro aprobado");
            }
            else {
                r.aprobada = false;
                snprintf(r.mensaje, 100, "Fondos insuficientes");
            }
        }
        else {                                     // Consulta
            snprintf(r.mensaje, 100, "Consulta");
        }
        r.saldo_resultante = saldo;
        historial[total] = t;
        total++;

        MPI_Send(&r, sizeof(RespuestaServidor), MPI_BYTE, estado.MPI_SOURCE, TAG_RESPUESTA, MPI_COMM_WORLD);

        if (detalle) {
            snprintf(linea, 300, "[Equipo: %s] [Nodo MPI: 0 Servidor] [Cliente: %d] [Hilo OpenMP: %d] [Op: %s] [Monto: $%.2f] [Estado: %s] [Saldo: $%.2f]",
                equipo.c_str(), t.id_cliente, t.id_hilo_openmp, nombreOp(t.tipo_operacion),
                t.monto, r.aprobada ? "Aprobado" : "Rechazado", saldo);
            registrar(linea, true);
        }
    }

    snprintf(linea, 300, "[SERVIDOR] Transacciones procesadas: %d | Saldo final: $%.2f", total, saldo);
    registrar(linea, true);
    delete[] historial;
}

// ======================== CLIENTES (NODOS 1..N-1) ========================
void cliente(int rank, int numOps, bool detalle) {
    // Arreglo transaccional con memoria dinamica
    Transaccion* lote = new Transaccion[numOps];
    RespuestaServidor* respuestas = new RespuestaServidor[numOps];

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        srand((unsigned)time(NULL) + rank * 100 + tid);   // semilla: tiempo + rank (+ hilo)

        // 1) Cada hilo genera su parte de las transacciones
#pragma omp for
        for (int i = 0; i < numOps; i++) {
            lote[i].id_cliente = rank;
            lote[i].id_hilo_openmp = tid;
            lote[i].tipo_operacion = rand() % 3 + 1;
            lote[i].monto = (lote[i].tipo_operacion == 3) ? 0 : rand() % 2000 + 100;
        }

        // 2) Cada hilo envia sus transacciones (un hilo a la vez usa MPI)
#pragma omp for
        for (int i = 0; i < numOps; i++) {
#pragma omp critical
            {
                MPI_Send(&lote[i], sizeof(Transaccion), MPI_BYTE, 0, TAG_TRANSACCION, MPI_COMM_WORLD);
                MPI_Recv(&respuestas[i], sizeof(RespuestaServidor), MPI_BYTE, 0, TAG_RESPUESTA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (detalle) {
                    char linea[300];
                    snprintf(linea, 300, "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] [Operacion: %s] [Monto: $%.2f] [Estado: %s] [Saldo Final: $%.2f]",
                        equipo.c_str(), rank, tid, nombreOp(lote[i].tipo_operacion), lote[i].monto,
                        respuestas[i].aprobada ? "Aprobado" : "Rechazado", respuestas[i].saldo_resultante);
                    registrar(linea, true);
                }
            }
        }
    }

    // Senal de termino (TAG_FIN)
    Transaccion fin;
    fin.id_cliente = rank; fin.id_hilo_openmp = 0; fin.tipo_operacion = 0; fin.monto = 0;
    RespuestaServidor r;
    MPI_Send(&fin, sizeof(Transaccion), MPI_BYTE, 0, TAG_FIN, MPI_COMM_WORLD);
    MPI_Recv(&r, sizeof(RespuestaServidor), MPI_BYTE, 0, TAG_RESPUESTA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    registrar("[Proceso MPI: " + to_string(rank) + "] Termino de enviar " + to_string(numOps) + " operaciones", false);

    delete[] lote;
    delete[] respuestas;
}

// ================================== MAIN ==================================
int main(int argc, char* argv[]) {
    int provided, rank, size, len;
    char host[MPI_MAX_PROCESSOR_NAME];

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(host, &len);
    equipo = host;

    if (rank == 0) imprimirNombres();   // PRIMERA linea de la ejecucion

    // Log local: log_equipo_[NOMBRE_PC]_nodo_[RANK].txt
    archivoLog.open("log_equipo_" + equipo + "_nodo_" + to_string(rank) + ".txt", ios::app);
    archivoLog << "===== Integrantes: Hermosillo Prado Carlos, Montes de Oca del Risco Rafael Alejandro, Ramirez Andrade Uriel Ismael Guadalupe" << endl;
    archivoLog << "===== Equipo: " << equipo << " | Rango MPI: " << rank
        << " | Hilos OpenMP: " << omp_get_max_threads() << endl;

    double saldo = 10000.00;   // solo lo usa el servidor
    int opcion = 0;

    do {
        if (rank == 0) {
            cout << "\n1. Simulacion basica (5 operaciones por cliente)\n"
                << "2. Simulacion masiva (100,000 operaciones por cliente)\n"
                << "3. Consultar saldo final del servidor\n"
                << "4. Salir\nOpcion: ";
            cin >> opcion;
        }
        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (opcion == 1 || opcion == 2) {
            int numOps = (opcion == 1) ? 5 : 100000;
            bool detalle = (opcion == 1);   // en la masiva no se imprime cada operacion

            registrar(detalle ? "\n--- SIMULACION BASICA ---" : "\n--- SIMULACION MASIVA ---", rank == 0);

            MPI_Barrier(MPI_COMM_WORLD);
            double inicio = MPI_Wtime();

            if (rank == 0) servidor(size, numOps, saldo, detalle);
            else cliente(rank, numOps, detalle);

            MPI_Barrier(MPI_COMM_WORLD);
            double tiempo = MPI_Wtime() - inicio;

            char linea[200];
            snprintf(linea, 200, "[Equipo: %s] [Nodo MPI: %d] Tiempo total: %.4f segundos", host, rank, tiempo);
            registrar(linea, rank == 0);
        }
        else if (opcion == 3 && rank == 0) {
            char linea[100];
            snprintf(linea, 100, "Saldo final del servidor: $%.2f", saldo);
            registrar(linea, true);
        }
    } while (opcion != 4);

    archivoLog.close();
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) imprimirNombres();   // ULTIMA linea de la ejecucion

    MPI_Finalize();
    return 0;
}