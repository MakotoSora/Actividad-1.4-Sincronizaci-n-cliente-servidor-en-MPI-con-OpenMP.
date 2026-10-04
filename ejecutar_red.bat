@echo off
REM ============================================================
REM  Practica 1.4 - Ejecucion DISTRIBUIDA EN RED (100%)
REM  Cluster de 3 PCs con MS-MPI (5 procesos en total):
REM    192.168.137.1   -> 1 proceso  (rank 0 = SERVIDOR)
REM    192.168.137.165 -> 2 procesos (clientes)
REM    192.168.137.153 -> 2 procesos (clientes)
REM
REM  Antes de ejecutar, en CADA PC:
REM    1. Servicio de MS-MPI activo (smpd -d o MsMpiLaunchSvc)
REM    2. practica1_4.exe en C:\MPI\Proyecto_MPI
REM    3. Firewall permitiendo mpiexec, smpd y practica1_4.exe
REM
REM  Equivalente en OpenMPI/MPICH: mpirun --hostfile hosts.txt -np 5 ./practica1_4
REM ============================================================
cd /d C:\MPI\Proyecto_MPI
mpiexec -hosts 3 192.168.137.1 1 192.168.137.165 2 192.168.137.153 2 C:\MPI\Proyecto_MPI\practica1_4.exe
