#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "json_extraction.h"

#define MAX_PARAM_SIZE 100

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

void send_string(char* string, int string_lenght, int destination) {
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

void start_master(struct json_data* data, char* path, char* epilogue_command) {
    int world_size = get_world_size();
    int ready_status = 0;
    int free_node;

    int start_index = extract_attribute(data, "params");

    jsmntok_t param_token = data->t[start_index];

    int str_len_base = strlen(path);

    char* command = malloc(sizeof(char) * (str_len_base + MAX_PARAM_SIZE + 1));
    strncpy(command, path, str_len_base);
    command[str_len_base] = ' ';
    for (int task_number = 0; task_number < param_token.size; task_number++) {
        jsmntok_t task_token = data->t[start_index + 1 + task_number];
        int str_len_param = task_token.end - task_token.start;
        extract_string(data, start_index + 1 + task_number, str_len_param,
                       command + str_len_base + 1);

        MPI_Recv(&free_node, 1, MPI_INT, MPI_ANY_SOURCE, 42, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        assert(free_node > 0 && free_node < world_size);
        fprintf(stderr,
                "\033[31m[MASTER]\033[39m Running task %d/%d (%d %%) on node "
                "%d ('%s')\n",
                task_number + 1, param_token.size,
                (task_number + 1) * 100 / param_token.size, free_node, command);
        MPI_Send(&ready_status, 1, MPI_INT, free_node, 99, MPI_COMM_WORLD);
        send_string(command, str_len_base + str_len_param + 1, free_node);
    }
    free(command);

    // Tell the nodes it is over and execute the epilogue if needed
    int end_status = (epilogue_command == NULL) ? -2 : -1;
    for (int i = 1; i < world_size; i++) {
        fprintf(stderr,
                "\033[31m[MASTER]\033[39m Tell Node %d the campaign is over: "
                "Execute epilogue\n",
                i);
        MPI_Send(&end_status, 1, MPI_INT, i, 99, MPI_COMM_WORLD);
        if (epilogue_command != NULL)
            send_string(epilogue_command, strlen(epilogue_command) + 1, i);
    }
}

void master_distribute_prologue(char* command) {
    int world_size = get_world_size();
    int ready_status = 0;
    for (int i = 1; i < world_size; i++) {
        fprintf(stderr,
                "\033[31m[MASTER]\033[39m Sending prologue to Node %d\n", i);
        MPI_Send(&ready_status, 1, MPI_INT, i, 41, MPI_COMM_WORLD);
        send_string(command, strlen(command) + 1, i);
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

void slave_execute_prologue(int my_rank) {
    int status;
    MPI_Recv(&status, 1, MPI_INT, 0, 41, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    assert(status == 0);
    fprintf(stderr, "\033[%dm[Node %d]\033[39m Executing prologue\n",
            (my_rank % 5) + 31, my_rank);
    slave_execute_command();
}

void tell_the_boss_i_am_free(int my_rank) {
    fprintf(stderr, "\033[%dm[Node %d]\033[39m Telling boss i am free\n",
            (my_rank % 5) + 31, my_rank);
    MPI_Send(&my_rank, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
}

void start_slave(int my_rank) {
    tell_the_boss_i_am_free(my_rank);
    int status;
    MPI_Recv(&status, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (status == 0) {
        fprintf(stderr, "\033[%dm[Node %d]\033[39m Executing job\n",
                (my_rank % 5) + 31, my_rank);
        slave_execute_command();
        start_slave(my_rank);
    } else {
        if (status == -1) {
            fprintf(stderr, "\033[%dm[Node %d]\033[39m Executing Epilogue\n",
                    (my_rank % 5) + 31, my_rank);
            slave_execute_command();
        }
        fprintf(stderr, "\033[%dm[Node %d]\033[39m Ok ! Going back home !\n",
                (my_rank % 5) + 31, my_rank);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Bad usage: mpirun [MPI Flags] %s [JSON FILE]\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    MPI_Init(&argc, &argv);
    int my_rank = get_my_rank();
    char* filename = argv[1];
    char* prologue = NULL;
    char* epilogue = NULL;

    struct json_data* data = extract_file(filename);

    char* path = extract_exec_file(data);
    prologue = extract_prologue(data);
    epilogue = extract_epilogue(data);

    if (my_rank == 0) {
        if (prologue != NULL)
            master_distribute_prologue(prologue);
        start_master(data, path, epilogue);
    } else {
        if (prologue != NULL)
            slave_execute_prologue(my_rank);
        start_slave(my_rank);
    }

    free_json_data(data);
    MPI_Finalize();
    return 0;
}
