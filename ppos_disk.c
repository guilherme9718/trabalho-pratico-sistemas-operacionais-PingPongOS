#include "ppos_disk.h"

// estrutura que define um tratador de sinal (deve ser global ou static)
static struct sigaction disk_action ;
int disk_wait;

int disk_scheduler();
void gerenciadora();

void tratador_disco(int signum) {
    disk.pacotes++;
    sem_up(&disk.cheio);
    disk.tempo_exec += systemTime - disk.tempo_init;
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {
    
    if(disk_cmd(DISK_CMD_INIT, 0, 0)) {
        printf("Falha ao iniciar o disco\n");
        return -1;
    }


    disk.tam = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if(disk.tam < 0) {
        printf("Falha ao consultar o tamanho do disco\n");
        return -1;
    }

    disk.tam_block = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if(disk.tam_block < 0) {
        printf("Falha ao consultar o tamanho do block\n");
        return -1;
    }

    *numBlocks = disk.tam;
    *blockSize = disk.tam_block;

    disk.init = 1;
    disk_wait = 0;
    
    mutex_create(&disk.mrequest);
    mutex_create(&disk.queue_mutex);
    sem_create(&disk.cheio, 0);
    sem_create(&disk.vazio, 0);

    disk.blocos_percorridos = 0;
    disk.tempo_exec = 0;

    disk.pacotes = 0;

    disk_queue = NULL;

    disk_action.sa_handler = tratador_disco;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;
    if(sigaction(SIGUSR1, &disk_action, 0) < 0) {
        printf("Erro em sigaction");
        return -1;
    }

    disk.sched = 2;

    task_create(&disk.task, gerenciadora, NULL);
    disk.task.privilegio = 0;
    
    return 0;
}

int disk_request(int block, void *buffer, int cmd) {
    dreq_t *request;
    request = (dreq_t*)malloc(sizeof(dreq_t));

    request->block = block;
    request->buffer = buffer;
    request->cmd = cmd;

    mutex_lock(&disk.queue_mutex);
    queue_append((queue_t**)&disk_queue, (queue_t*)request);
    mutex_unlock(&disk.queue_mutex);

    return 0;
}


void gerenciadora() {
    while(disk.init == 1) {
        sem_down(&disk.vazio);
        disk_scheduler();
        
    }
    printf("Numero de blocos percorridos %d\nTempo de execução do disco %d ms\n", 
    disk.blocos_percorridos, disk.tempo_exec);
    task_exit(0);
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
    int cmd = DISK_CMD_READ;
    
    disk_request(block, buffer, cmd);
    
    mutex_lock(&disk.mrequest);

    
    if(disk.pacotes == 0) {
        sem_up(&disk.vazio);
        sem_down(&disk.cheio);
    }
    disk.pacotes--;
    
    mutex_unlock(&disk.mrequest);
    return 0;
}



// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    int cmd = DISK_CMD_WRITE;
    
    //coloca tarefa na fila para acessar o disco
    disk_request(block, buffer, cmd);

    mutex_lock(&disk.mrequest);

    if(disk.pacotes == 0) {
        sem_up(&disk.vazio);
        sem_down(&disk.cheio);
    }
    disk.pacotes = 0;
    
    mutex_unlock(&disk.mrequest);

    return 0;
}

dreq_t* fcfs_sched() {
    int size = queue_size((queue_t*)disk_queue);
    int i=0;
    dreq_t* iter=NULL;

    return disk_queue;
}

dreq_t* sstf_sched() {
    int size = queue_size((queue_t*)disk_queue);
    int i=0;
    dreq_t* iter=disk_queue;

    int head = disk.bloco;
    int menor = abs(iter->block - head);
    dreq_t* menor_req = iter;

    int aux;
    for(i=0, iter=NULL; i < size && iter != NULL; i++, iter=iter->next) {
        aux = abs(iter->block - head);
        if(aux < menor) {
            menor = aux;
            menor_req = iter;
        }
    }

    return menor_req;
}

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
    for(i=0, iter=NULL; i < size && iter != NULL; i++, iter=iter->next) {
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

int disk_scheduler() {
    if(disk_queue == NULL) {
        return 0;
    }
    dreq_t* req, *aux;

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

    int req_block = req->block;
    void *req_buffer = req->buffer;
    int req_cmd = req->cmd;
    
    aux = (dreq_t*)queue_remove((queue_t**)&disk_queue, (queue_t*)req);
    free(req);

    
    disk.status = disk_cmd(DISK_CMD_STATUS, 0, 0);
    if(disk.status != DISK_STATUS_IDLE) {
        printf("disk_status = %d\n", disk.status);
        return -1;
    }
    disk.tempo_init = systemTime;
    disk.blocos_percorridos += abs(req_block - disk.bloco);

    disk.bloco = req_block;
    disk.buffer = req_buffer;

    int result = disk_cmd(req_cmd, disk.bloco, disk.buffer);
    if(result != 0) {
        printf("Falha ao ler/escrever o bloco %d %d %p %d %d\n", disk.bloco, result, disk.buffer, disk.tam, disk.status);
        sem_up(&disk.cheio);
        return -1;
    }

}