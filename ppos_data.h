// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

/* Implementação feita por
Guilherme Aguilar de Oliveira
RA 2127953

Samuel Leal Valentin
RA 2023989
*/

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <stdio.h>
#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   unsigned char state;  // indica o estado de uma tarefa (ver defines no final do arquivo ppos.h): 
                          // n - nova, r - pronta, x - executando, s - suspensa, e - terminada
   struct task_t* queue;
   struct task_t* joinQueue;
   int exitCode;
   unsigned int awakeTime; // used to store the time when it should be waked up

   // ... (outros campos deve ser adicionados APOS esse comentario)
   int static_prio; // armazena a prioridade estatica da tarefa
   int dinamic_prio; // armazena a prioridade ganhada por envelhecimento
   unsigned char privilegio; // 1 se o privilegio for de usuario e 0 se o privilegio for de sistema

   //metricas da tarefa
   unsigned int executionTimeStart; // grava o tempo de inicio da tarefa
   unsigned int processorTime; // grava o tempo de processamento da tarefa
   unsigned int processorTimeStart; // grava o tempo de inicio que a tarefa recebe o processador
   unsigned int activations; // grava o tanto de ativações da tarefa

   int quantum; // tempo de processamento em milissegundos
} task_t ;

// estrutura que define um semáforo
typedef struct {
    struct task_t *queue;
    int value;

    unsigned char active;
} semaphore_t ;

// estrutura que define um mutex
typedef struct {
    struct task_t *queue;
    unsigned char value;

    unsigned char active;
} mutex_t ;

// estrutura que define uma barreira
typedef struct {
    struct task_t *queue;
    int maxTasks;
    int countTasks;
    unsigned char active;
    mutex_t mutex;
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct {
    void* content;
    int messageSize;
    int maxMessages;
    int countMessages;
    
    semaphore_t sBuffer;
    semaphore_t sItem;
    semaphore_t sVaga;
    
    unsigned char active;
} mqueue_t ;

#endif

