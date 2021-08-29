#include "ppos_disk.h"
/* Implementação feita por
Guilherme Aguilar de Oliveira
RA 2127953

Samuel Leal Valentin
RA 2023989
*/

// estrutura que define um tratador de sinal (deve ser global ou static)
static struct sigaction disk_action ;
int disk_wait;

int disk_scheduler();
void gerenciadora();

//Função para tratar o sinais retornados do disco
void tratador_disco(int signum) {
    //atualiza o numero de pacotes
    disk.pacotes++;
    //libera a espera de pacotes pelas tarefas
    sem_up(&disk.cheio);
    //calcula o tempo de execução do disco
    disk.tempo_exec += systemTime - disk.tempo_init;
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {
    //inicia o disco e verifica se deu certo
    if(disk_cmd(DISK_CMD_INIT, 0, 0)) {
        printf("Falha ao iniciar o disco\n");
        return -1;
    }

    //verifica o tamanho do disco
    disk.tam = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if(disk.tam < 0) {
        printf("Falha ao consultar o tamanho do disco\n");
        return -1;
    }

    //verifica o tamanho do bloco
    disk.tam_block = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if(disk.tam_block < 0) {
        printf("Falha ao consultar o tamanho do block\n");
        return -1;
    }

    *numBlocks = disk.tam;
    *blockSize = disk.tam_block;

    disk.init = 1;
    
    //inicializa os mutexes e semáforos
    mutex_create(&disk.mrequest);
    mutex_create(&disk.queue_mutex);
    sem_create(&disk.cheio, 0);
    sem_create(&disk.vazio, 0);

    //inicializa variáveis de controle
    disk.blocos_percorridos = 0;
    disk.tempo_exec = 0;

    disk.pacotes = 0;

    disk_queue = NULL;

    //inicializa função tratadora do sinal SIGUSR1
    disk_action.sa_handler = tratador_disco;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;
    if(sigaction(SIGUSR1, &disk_action, 0) < 0) {
        printf("Erro em sigaction");
        return -1;
    }

    //Escolhe a política de escalonamento de disco
    disk.sched = DISK_FCFS;
    //disk.sched = DISK_SSTF;
    //disk.sched = DISK_CSCAN;

    //cria tarefa gerenciadora do disco
    task_create(&disk.task, gerenciadora, NULL);
    //define o privilégio de sistema para a tarefa gerenciadora de disco
    disk.task.privilegio = 0;
    
    return 0;
}

//Função para colocar na fila de solicitações de acesso ao disco
int disk_request(int block, void *buffer, int cmd) {
    dreq_t *request;
    request = (dreq_t*)malloc(sizeof(dreq_t));

    request->block = block;
    request->buffer = buffer;
    request->cmd = cmd;

    //acesso mutuamente exclusivo à fila de solicitações
    mutex_lock(&disk.queue_mutex);
    queue_append((queue_t**)&disk_queue, (queue_t*)request);
    mutex_unlock(&disk.queue_mutex);

    return 0;
}

//Função da tarefa gerenciadora do disco
void gerenciadora() {
    //enquanto a task main não terminar
    while(taskMain->state != 'e' && disk.init == 1) {
        //espera pedidos de acesso ao disco
        sem_down(&disk.vazio);
        //escalona um pedido de acesso
        disk_scheduler();
    }
    printf("Numero de blocos percorridos %d\nTempo de execução do disco %d ms\n", 
    disk.blocos_percorridos, disk.tempo_exec);
    task_exit(0);
}

// leitura de um bloco, do disco para o buffer
//implementado no formato do jantar dos selvagens
int disk_block_read (int block, void *buffer) {
    int cmd = DISK_CMD_READ;
    //coloca solicitação na fila
    disk_request(block, buffer, cmd);
    
    //pede acesso ao disco
    mutex_lock(&disk.mrequest);

    
    if(disk.pacotes == 0) {
        //acorda a tarefa gerenciadora
        sem_up(&disk.vazio);
        //espera o disco escrever no buffer
        sem_down(&disk.cheio);
    }
    //consome o pacote retornado pelo disco
    disk.pacotes--;
    
    mutex_unlock(&disk.mrequest);
    return 0;
}



// escrita de um bloco, do buffer para o disco
//implementado no formato do jantar dos selvagens
int disk_block_write (int block, void *buffer) {
    int cmd = DISK_CMD_WRITE;
    
    //coloca solicitação na fila
    disk_request(block, buffer, cmd);

    //pede acesso ao disco
    mutex_lock(&disk.mrequest);

    if(disk.pacotes == 0) {
        //acorda a tarefa gerenciadora
        sem_up(&disk.vazio);
        //espera o disco escrever no buffer
        sem_down(&disk.cheio);
    }
    //consome o pacote retornado pelo disco
    disk.pacotes--;
    
    mutex_unlock(&disk.mrequest);

    return 0;
}

//escalonamento FCFS: retorna a primeira solicitação da fila
dreq_t* fcfs_sched() {
    int i=0;
    dreq_t* iter=NULL;

    return disk_queue;
}

//escalonamento SSTF: retorna a solicitação mais próxima da cabeça do disco
dreq_t* sstf_sched() {
    int size = queue_size((queue_t*)disk_queue);
    int i=0;
    dreq_t* iter=disk_queue;

    int head = disk.bloco;
    int menor = abs(iter->block - head);
    dreq_t* menor_req = iter;

    int aux;
    for(i=0, iter=NULL; i < size && iter != NULL; i++, iter=(dreq_t*)iter->next) {
        aux = abs(iter->block - head);
        if(aux < menor) {
            menor = aux;
            menor_req = iter;
        }
    }

    return menor_req;
}

//escalonamento CSCAN: retorna a tarefa mais próxima à frente da cabeça do disco.
//se não houver tarefas a frente, retorna a tarefa mais próxima do ínicio do disco (movimento circular).
dreq_t* cscan_sched() {
    int size = queue_size((queue_t*)disk_queue);
    int i=0;
    dreq_t* iter=disk_queue;

    int head = disk.bloco;
    int menor_frente = iter->block - head;
    int menor = iter->block;
    dreq_t* menor_req = iter, *menor_req2 = iter;

    int aux, aux2;
    int frente=0;
    for(i=0, iter=NULL; i < size && iter != NULL; i++, iter=(dreq_t*)iter->next) {
        aux = iter->block - head;
        if(aux >= 0) {
            frente = 1;
            if(aux < menor_frente) {
                menor_frente = aux;
                menor_req = iter;
            }
        }
        else {
            aux = iter->block;
            if(aux < menor) {
                menor = aux;
                menor_req2 = iter;
            }
        }
    }

    if(frente) {
        return menor_req;
    }
    else {
        return menor_req2;
    }
}

//escalona os pedidos de acordo com a política definida e envia solicitação para o disco
int disk_scheduler() {
    //se não houver tarefa na fila, retorna zero
    if(disk_queue == NULL) {
        return 0;
    }
    dreq_t* req, *aux;

    //escalona as solicitações de acordo com a política 
    switch(disk.sched) {
        case 0 :
        req = fcfs_sched();
        break;

        case 1 :
        req = sstf_sched();
        break;

        case 2 :
        req = cscan_sched();
        break;

        default :
        req = fcfs_sched();
    }

    //copia os dados do pedido escalonada
    int req_block = req->block;
    void *req_buffer = req->buffer;
    int req_cmd = req->cmd;
    
    //verifica o status do disco. se o disco estiver ocupado retorna erro
    disk.status = disk_cmd(DISK_CMD_STATUS, 0, 0);
    if(disk.status != DISK_STATUS_IDLE) {
        printf("disk_status = %d\n", disk.status);
        return -1;
    }

    //envia solicitação para o disco e verifica
    int result = disk_cmd(req_cmd, req_block, req_buffer);
    if(result != 0) {
        printf("Falha ao ler/escrever o bloco %d %d %p %d %d\n", disk.bloco, result, disk.buffer, disk.tam, disk.status);
        sem_up(&disk.cheio);
        return -1;
    }

    //mede o tempo de atividade do disco e blocos percorridos
    disk.tempo_init = systemTime;
    disk.blocos_percorridos += abs(req_block - disk.bloco);

    //atuliza a posição da cabeça do disco
    disk.bloco = req_block;
    disk.buffer = req_buffer;

    //libera solicitação
    aux = (dreq_t*)queue_remove((queue_t**)&disk_queue, (queue_t*)req);
    free(req);

}