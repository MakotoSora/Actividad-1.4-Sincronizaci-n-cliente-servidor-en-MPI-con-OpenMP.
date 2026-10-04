# Práctica 1.4: Sincronización Cliente-Servidor en MPI con OpenMP

**Integrantes:** Hermosillo Prado Carlos · Montes de Oca del Risco Rafael Alejandro · Ramírez Andrade Uriel Ismael Guadalupe

Cuenta bancaria compartida bajo el modelo Cliente-Servidor:

- **Servidor (nodo 0):** único dueño de `saldo_cuenta` (inicia en $10,000.00). Atiende las peticiones una por una con `MPI_Recv(MPI_ANY_SOURCE, MPI_ANY_TAG)`. Esa recepción secuencial garantiza la exclusión mutua sin candados ni memoria compartida.
- **Clientes (nodos 1..N-1):** con `#pragma omp parallel` generan un arreglo dinámico de transacciones aleatorias (depósito, retiro, consulta). Después las envían al servidor (`MPI_Send`) y esperan la respuesta (`MPI_Recv`) dentro de una zona `omp critical`.

## Tags MPI
| Tag | Dirección | Uso |
|---|---|---|
| `TAG_TRANSACCION` (1) | Cliente → Servidor | Petición |
| `TAG_FIN` (2) | Cliente → Servidor | Señal de término |
| `TAG_RESPUESTA` (3) | Servidor → Cliente | Respuesta (aprobada/rechazada + saldo) |

## Archivos
| Archivo | Contenido |
|---|---|
| `practica1_4.cpp` | Código completo (servidor, clientes, estructuras, logger, menú) |
| `compilar.bat` | Compilación con MSVC + MS-MPI + OpenMP |
| `ejecutar_local.bat` | 5 procesos en una PC (1 servidor + 4 clientes) |
| `ejecutar_red.bat` | Ejecución en clúster de 3 PCs (192.168.137.1 servidor; .165 y .153 clientes) |

## Compilar
En **Developer Command Prompt for VS (x64)**, dentro de `C:\MPI\Proyecto_MPI`:
```
cl /EHsc /O2 /openmp /I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" practica1_4.cpp /link /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" msmpi.lib /out:practica1_4.exe
```
(o simplemente `compilar.bat`)

## Ejecutar
Local (1 servidor + 4 clientes):
```
mpiexec -n 5 C:\MPI\Proyecto_MPI\practica1_4.exe
```
En red (3 PCs, 5 procesos; la primera IP recibe el rank 0 = servidor):
```
mpiexec -hosts 3 192.168.137.1 1 192.168.137.165 2 192.168.137.153 2 C:\MPI\Proyecto_MPI\practica1_4.exe
```

Menú (opción difundida con `MPI_Bcast`):
1. Simulación básica: 5 operaciones por cliente, salida detallada.
2. Simulación masiva: 100,000 operaciones por cliente, solo tiempo, total y saldo.
3. Consultar saldo final del servidor.
4. Salir.

## Logs
Cada nodo crea `log_equipo_[NOMBRE_PC]_nodo_[RANK].txt` en `C:\MPI\Proyecto_MPI` de su propia PC. El archivo se abre en modo *append*, así que conserva todas las simulaciones.
