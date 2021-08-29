// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

/* Implementação feita por
Guilherme Aguilar de Oliveira
RA 2127953

Samuel Leal Valentin
RA 2023989
*/

#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include <signal.h>
#include <sys/time.h>
#include <math.h>

#define DISK_FCFS 0
#define DISK_SSTF 1
#define DISK_CSCAN 2

//Estrutura para fazer fila de solicitações de acesso ao disco
typedef struct {
  struct dreq_t *prev, *next;
  int cmd;
  int block;
  void *buffer;
} dreq_t;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  //variável se o disco está inicializado, 0 - disco offline, 1 - disco online
  char init;
  //numero de blocos do disco
  int tam;
  //tamanho de cada bloco do disco
  int tam_block;
  //posição atual da cabeça do disco
  int bloco;
  //buffer a ser gravado da solicitação escalonada
  void* buffer;
  //status do disco
  int status;
  //tarefa gerenciadora do disco
  task_t task;
  //solicitação escalonada
  dreq_t req;
  //mutex de acesso ao disco
  mutex_t mrequest;
  //mutex de acesso a fila de solicitações ao disco
  mutex_t queue_mutex;
  //semáforo para acordar a tarefa gerenciadora (jantar dos selvagens)
  semaphore_t vazio;
  //semáforo para a tarefas esperarem o disco encher o buffer (jantar dos selvagens)
  semaphore_t cheio;
  //numero de pacotes recebidos do disco
  short pacotes;
  //política de escalonamento do disco
  int sched;
  //variável para calcular o tempo de execução do disco
  int tempo_init;
  //número de blocos percorridos do disco
  int blocos_percorridos;
  //tempo de execução do disco
  int tempo_exec;
  // completar com os campos necessarios
} disk_t ;


disk_t disk;
dreq_t* disk_queue;
// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif
