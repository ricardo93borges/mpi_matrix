#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <mpi.h>
#include <omp.h>

int m1[SIZE][SIZE],m2[SIZE][SIZE],mres[SIZE][SIZE];
int l1, c1, l2, c2, lres, cres;

void initializeMatrices() {
    int i, j, k;

    l1 = c1 = SIZE;
    l2 = c2 = SIZE;
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
}

int validateMatrix(){
    int i, j, k;
    for (i=0 ; i<SIZE; i++) {
        k = SIZE*(i+1);
        for (j=0 ; j<SIZE; j++) {
            int k_col = k*(j+1);
            if (i % 2 ==0) {
                if (j % 2 == 0) {
                    if (mres[i][j]!=k_col){
                        printf("\nF1");
                        MPI_Finalize();
                        return 0;
                    }
                }
                else {
                    if (mres[i][j]!=-k_col){
                        printf("\nF2");
                        MPI_Finalize();
                        return 0;
                    }
                }
            }
            else {
                if (j % 2 == 0) {
                    if (mres[i][j]!=-k_col){
                        printf("\nF3");
                        MPI_Finalize();
                        return 0;
                    }
                }
                else {
                    if (mres[i][j]!=k_col){
                        printf("\nF4");
                        MPI_Finalize();
                        return 0;
                    }
                }
            }
        } 
    }
    return 1;
}

int main(int argc, char *argv[]) {

    int    i, j, k, n, f, rank, nprocs, name_len, offset, rows_per_proccess;
    double elapsed_time;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);

    // PREPARA PARA MEDIR TEMPO
    elapsed_time = - MPI_Wtime ();
    if(rank == 0){

        // INICIALIZA OS ARRAYS A SEREM MULTIPLICADOS
        initializeMatrices();

        //printf("\n rank: %d", rank);
        offset = 0;
        for(n=1; n < nprocs; n++){

            //Send name
            MPI_Send(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, n, 1, MPI_COMM_WORLD);

            //Send second matrix
            MPI_Send(&m2, SIZE*SIZE, MPI_INT, n, 2, MPI_COMM_WORLD);

            //Send offset
            MPI_Send(&offset, 1, MPI_INT, n, 3, MPI_COMM_WORLD);
            
            //Send rows per proccess
            rows_per_proccess = SIZE / (nprocs-1);
            if(n == (nprocs-1) && (rows_per_proccess * (nprocs-1)) < SIZE ){
                rows_per_proccess = SIZE-offset;   
            }
            
            int rows[rows_per_proccess][SIZE];
            MPI_Send(&rows_per_proccess, 1, MPI_INT, n, 4, MPI_COMM_WORLD);

            //Send first matrix rows                
            k = 0;
            for(i=offset; i < (offset+rows_per_proccess); i++){
                for(j=0; j<SIZE; j++) {
                    rows[k][j] = m1[i][j];
                }
                k++;
            }


            /* for (i=0 ; i<SIZE; i++) {
                for (j=0 ; j<SIZE; j++) {
                    printf(" %d ", rows[i][j]);
                    if (j == SIZE-1) printf("\n");
                }
            } */

            MPI_Send(&rows, rows_per_proccess*SIZE, MPI_INT, n, 5, MPI_COMM_WORLD);

            offset = offset+rows_per_proccess;
        }

        int slave_offset;
        for(n=1; n < nprocs; n++){
            //Receive offset
            MPI_Recv(&slave_offset, 1, MPI_INT, n, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            //Receive rows per process
            MPI_Recv(&rows_per_proccess, 1, MPI_INT, n, 9, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("\n n: %d, offset: %d, rows per process: %d", n, slave_offset, rows_per_proccess);
            
            //Receive result
            int res[rows_per_proccess][SIZE];
            MPI_Recv(&res, rows_per_proccess*SIZE, MPI_INT, n, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            //Insert results into mres matrix
            for(i=0; i < rows_per_proccess; i++){
                for(j=0; j<SIZE; j++) mres[slave_offset+i][j] = res[i][j];
            }
        }

        /* for (i=0 ; i<SIZE; i++) {
            for (j=0 ; j<SIZE; j++) {
                printf(" %d ", mres[i][j]);
                if (j == SIZE-1) printf("\n");
            }
        } */


        // VERIFICA SE O RESULTADO DA MULTIPLICACAO ESTA CORRETO
        if(!validateMatrix()){
            MPI_Finalize();
            return 1;
        }

        // OBTEM O TEMPO
        elapsed_time += MPI_Wtime ();

        // MOSTRA O TEMPO DE EXECUCAO
        printf("\nelapsed_time: %lf",elapsed_time);

    }else{
        int cores;
        char slave_processor_name[MPI_MAX_PROCESSOR_NAME];
        //Receive name
        MPI_Recv(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Receive sencond matrix
        MPI_Recv(&m2, SIZE*SIZE, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Receive offset
        MPI_Recv(&offset, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Receive rows per proccess
        MPI_Recv(&rows_per_proccess, 1, MPI_INT, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("\n rank: %d, offset: %d, rows: %d", rank, offset, rows_per_proccess);

        //Receive first matrix rows
        int rows[rows_per_proccess][SIZE];
        int msres[rows_per_proccess][SIZE];
        MPI_Recv(&rows, rows_per_proccess*SIZE, MPI_INT, 0, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Set how many cores to use
        MPI_Get_processor_name(slave_processor_name, &name_len);
        cores =  get_nprocs();
        if (strcmp(processor_name, slave_processor_name) == 0) cores =  get_nprocs()-1;

        omp_set_num_threads(cores);

        // REALIZA A MULTIPLICACAO
        # pragma omp parallel for private (i,j)
        for (i=0 ; i<rows_per_proccess; i++) {
            for (j=0 ; j<SIZE; j++) {
                # pragma omp critical 
                {
                    msres[i][j] = 0;
                    for (k=0 ; k<SIZE; k++) {                        
                        msres[i][j] += rows[i][k] * m2[k][j];
                    }
                }
            }
        }

        //Send offset to master
        MPI_Send(&offset, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);

        //Send rows per process
        MPI_Send(&rows_per_proccess, 1, MPI_INT, 0, 9, MPI_COMM_WORLD);

        //Send msres to master
        MPI_Send(&msres, rows_per_proccess*SIZE, MPI_INT, 0, 6, MPI_COMM_WORLD);
    }

    printf("\n >> Rank %d finalizing", rank);
    MPI_Finalize();
    return 0;
}

