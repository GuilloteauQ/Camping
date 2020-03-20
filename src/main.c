#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int get_my_rank() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
}

int get_world_size() {
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    return size;
}

void send_string(char* string, int destination) {
    int string_lenght = strlen(string);
    // First sending the size of the string
    // to allocate the correct size
    MPI_Send(&string_lenght, 1, MPI_INT, destination, 99, MPI_COMM_WORLD);
    // Then sending the string
    MPI_Send(string, string_lenght, MPI_CHAR, destination, 99, MPI_COMM_WORLD);
}

char* receive_string(int source, int* string_lenght) {
    MPI_Recv(string_lenght, 1, MPI_INT, source, 99, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    char* string = malloc(sizeof(char) * (*string_lenght));
    MPI_Recv(string, *string_lenght, MPI_CHAR, source, 99, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    return string;
}

void tell_him_i_am_ready(int destination) {
    MPI_Send(0, 1, MPI_INT, destination, 99, MPI_COMM_WORLD);
}

struct queue {
    int node;
    struct queue* next;
};

void print_queue(struct queue* q) {
    if (q != NULL) {
        printf("%d->", q->node);
        print_queue(q->next);
    } else {
        printf("NULL\n");
    }
}

void push(struct queue** q, int node) {
    // printf("PUSHING %d into: ", node);
    // print_queue(*q);
    struct queue* new_item = malloc(sizeof(struct queue*));
    new_item->node = node;
    new_item->next = NULL;
    if (*q == NULL) {
        *q = new_item;
    } else {
        struct queue* current = *q;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_item;
    }
    // printf("DONE PUSHING %d: ", node);
    // print_queue(*q);
}

int pop(struct queue** q) {
    // printf("POPPING FROM: ");
    // print_queue(*q);
    if (*q == NULL)
        return -1;
    int node = (*q)->node;
    struct queue* new_head = (*q)->next;
    free(*q);
    *q = new_head;
    // printf("POPPED %d, remaining: ", node);
    // print_queue(*q);
    // print_queue(new_head);
    return node;
}

int get_index_free_node(int world_size, struct queue** q) {
    int first_in_queue = pop(q);
    if (first_in_queue != -1)
        return first_in_queue;

    MPI_Request* reqs = malloc(sizeof(MPI_Request) * (world_size - 1));
    int* status = malloc(sizeof(int) * (world_size - 1));

    int flag;
    for (int i = 1; i < world_size; i++) {
        MPI_Irecv(&(status[i - 1]), 1, MPI_INT, i, 99, MPI_COMM_WORLD,
                  &reqs[i - 1]);
    }

    while (*q == NULL) {
        for (int i = 1; i < world_size; i++) {
            MPI_Request_get_status(reqs[i - 1], &flag, MPI_STATUS_IGNORE);
            // printf("node: %d, flag: %d\n", i, flag);
            if (flag) {
                if (status[i - 1] == 0) {
                    push(q, i);
                }
            }
        }
        sleep(10);
    }
    free(status);
    for (int j = 0; j < world_size - 1; j++)
        MPI_Request_free(&(reqs[j]));
    return pop(q);
}

void start_master(char* filename) {
    int world_size = get_world_size();
    int ready_status = 0;
    struct queue* q = NULL;
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        // printf("Retrieved line of length %zu:\n", read);
        // printf("%s", line);
        int free_node = get_index_free_node(world_size, &q);
        print_queue(q);
        printf("[MASTER] Running on node %d ('%s')\n", free_node, line);
        MPI_Send(&ready_status, 1, MPI_INT, free_node, 99, MPI_COMM_WORLD);
        send_string(line, free_node);
    }

    fclose(fp);
    if (line)
        free(line);

    // Tell the nodes it is over
    int end_status = -1;
    for (int i = 1; i < world_size; i++) {
        printf("[MASTER] Tell Node %d the campaign is over\n", i);
        MPI_Send(&end_status, 1, MPI_INT, i, 99, MPI_COMM_WORLD);
    }
}

void slave_execute_command() {
    // Receive the command as a string
    int string_lenght = 0;
    char* string = receive_string(0, &string_lenght);
    int status = system(string);
    assert(status != -1);
    // Execute the command
    // FILE* fp;
    // char* line = NULL;
    // size_t len = 0;
    // ssize_t read = 0;

    // fp = popen(string, "r");
    // if (fp == NULL)
    //     exit(EXIT_FAILURE);

    // // assert((read = getline(&line, &len, fp)) != -1);
    // int status = (read = getline(&line, &len, fp));

    // pclose(fp);

    // send_string(line, 0);

    // if (line)
    //     free(line);
    free(string);
}

void tell_boss(int status) {
    MPI_Request req;
    MPI_Isend(&status, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    // MPI_Send(&status, 1, MPI_INT, 0, 99, MPI_COMM_WORLD);
}

void tell_the_boss_i_am_free() {
    printf("[Node %d] Telling boss i am free\n", get_my_rank());
    tell_boss(0);
}

void tell_the_boss_i_am_busy() {
    tell_boss(-1);
}

void start_slave(int my_rank) {
    tell_the_boss_i_am_free();
    int status;
    MPI_Recv(&status, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (status == 0) {
        // tell_the_boss_i_am_busy();
        printf("[Node %d] Executing job\n", my_rank);
        slave_execute_command();
        start_slave(my_rank);
    } else {
        printf("[Node %d] Ok ! Going back home !\n", my_rank);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }
    MPI_Init(&argc, &argv);
    int my_rank = get_my_rank();

    if (my_rank == 0) {
        start_master(argv[1]);
    } else {
        start_slave(my_rank);
    }

    MPI_Finalize();
    return 0;
}
