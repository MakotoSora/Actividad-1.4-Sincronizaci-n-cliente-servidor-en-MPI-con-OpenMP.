@echo off
REM ============================================================
REM  Practica 1.4 - Ejecucion LOCAL (60%)
REM  1 servidor (nodo 0) + 4 clientes (nodos 1-4) en esta PC
REM ============================================================
cd /d C:\MPI\Proyecto_MPI
mpiexec -n 5 C:\MPI\Proyecto_MPI\practica1_4.exe
