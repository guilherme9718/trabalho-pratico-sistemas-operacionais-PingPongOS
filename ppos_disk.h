// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

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

typedef struct {
  struct dreq_t *prev, *next;
  //task_t task;
  int cmd;
  int block;
  void *buffer;
} dreq_t;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  char init;
  int tam;
  int tam_block;
  int bloco;
  void* buffer;
  int status;
  task_t task;
  dreq_t req;
  mutex_t mrequest;
  mutex_t queue_mutex;
  semaphore_t vazio;
  semaphore_t cheio;
  short pacotes;
  int sched;

  int tempo_init;
  int blocos_percorridos;
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
