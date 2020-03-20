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

void start_master(char* filename) {
    int world_size = get_world_size();
    int ready_status = 0;
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int free_node;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        MPI_Recv(&free_node, 1, MPI_INT, MPI_ANY_SOURCE, 42, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        assert(free_node > 0 && free_node < world_size);
        line[strcspn(line, "\n")] = 0;
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
    free(string);
}

void tell_the_boss_i_am_free(int my_rank) {
    printf("[Node %d] Telling boss i am free\n", get_my_rank());
    MPI_Send(&my_rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
}

void start_slave(int my_rank) {
    tell_the_boss_i_am_free(my_rank);
    int status;
    MPI_Recv(&status, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (status == 0) {
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
