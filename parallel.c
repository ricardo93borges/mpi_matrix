/**
 * TODO get machine name
 * TODO get machine cores
 * TODO define protocol to messages: 
 *  1 - send manchine name, 
 *  2 - send second matrix, 
 *  3 - send first matrix lines,
 *  4 - send results (from slave to master) 
 *  5 - finish signal
 * TODO master initialize matrix
 * TODO master send machine name to slave
 * TODO master send second matrix to slave
 * TODO master send first matrix lines to slave
 * TODO slave send multiplicated lines to master and number of initial line
 * TODO master allocate lines in the third matrix accordingly to inital line received
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <mpi.h>

int m1[SIZE][SIZE],m2[SIZE][SIZE],mres[SIZE][SIZE];
int l1, c1, l2, c2, lres, cres;

int main(int argc, char *argv[]) {

    int    i, j, k, n, rank, nprocs, name_len;
    double elapsed_time;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);

    // INICIALIZA OS ARRAYS A SEREM MULTIPLICADOS
    l1 = c1 = SIZE;
    l2 = c2 = SIZE;
    if (c1 != l2) {
        fprintf(stderr,"Impossivel multiplicar matrizes: parametros invalidos.\n");
        return 1;
    }
    lres = l1;
    cres = c2;
    k=1;
    for (i=0 ; i<SIZE; i++) {
        for (j=0 ; j<SIZE; j++) {
            if (k%2==0)
                m1[i][j] = -k;
            else
                m1[i][j] = k;
        }
        k++;
    }
    
    k=1;
    for (j=0 ; j<SIZE; j++) {
        for (i=0 ; i<SIZE; i++) {
            if (k%2==0)
                m2[i][j] = -k;
            else
                m2[i][j] = k;
        }
        k++;
    }

    // PREPARA PARA MEDIR TEMPO
    elapsed_time = - MPI_Wtime ();

    printf("\ncores: %d, nprocs: %d", get_nprocs(), nprocs);

    if( rank == 0){
        printf("\nmaster - rank: %d, processor_name: %s", rank, processor_name);

        for(n=1; n < nprocs; n++){
            //Send name
            MPI_Send(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, n, 1, MPI_COMM_WORLD);

            //Send second matrix
            MPI_Send(&m2, SIZE*SIZE, MPI_INT, n, 2, MPI_COMM_WORLD);

            //Send first matrix lines

            //Receive result
        }
    }else{
        int cores_to_use;
        char slave_processor_name[MPI_MAX_PROCESSOR_NAME];

        //Receive name
        MPI_Recv(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        //Set how many cores to use
        MPI_Get_processor_name(slave_processor_name, &name_len);
        if (strcmp (processor_name, slave_processor_name) == 0) {
            cores_to_use =  get_nprocs()-1;
        }else{
            cores_to_use =  get_nprocs();
        }

        printf("\nslave - rank: %d, processor_name: %s, master_name: %s, using %d cores", rank, slave_processor_name, processor_name, cores_to_use);

        //Receive sencond matrix
        MPI_Recv(&m2, SIZE*SIZE, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // OBTEM O TEMPO
    elapsed_time += MPI_Wtime ();

    // MOSTRA O TEMPO DE EXECUCAO
    printf("\n%lf",elapsed_time);
    printf("\n");
    MPI_Finalize();
    return 0;
}

