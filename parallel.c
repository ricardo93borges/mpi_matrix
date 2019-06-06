#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <mpi.h>
#include <omp.h>

int m1[SIZE][SIZE],m2[SIZE][SIZE],mres[SIZE][SIZE];
int l1, c1, l2, c2, lres, cres;

int main(int argc, char *argv[]) {

    int    i, j, k, n, rank, nprocs, name_len, offset, rows_per_proccess;
    double elapsed_time;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);

    // PREPARA PARA MEDIR TEMPO
    elapsed_time = - MPI_Wtime ();

    if( rank == 0){
        /**
         * MPI Tags
         *  1 - send manchine name, 
         *  2 - send second matrix, 
         *  3 - send offset,
         *  4 - send number of rows,
         *  5 - send rows
         *  6 - send results (from slave to master)
         *  7 - send finish signal
         *  8 - send number of cores (from slave to master)
        **/
        // INICIALIZA OS ARRAYS A SEREM MULTIPLICADOS
        l1 = c1 = SIZE;
        l2 = c2 = SIZE;
        if (c1 != l2) {
            fprintf(stderr,"Impossivel multiplicar matrizes: parametros invalidos.\n");
            MPI_Finalize();
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

        offset = 0;
        int slave_offset, cores;        
        while(1){
            for(n=1; n < nprocs; n++){                
                if(offset >= SIZE) {
                    break;
                }
                //Send name
                MPI_Send(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, n, 1, MPI_COMM_WORLD);

                //Receive number of cores
                MPI_Recv(&cores, 1, MPI_INT, n, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //Send second matrix
                MPI_Send(&m2, SIZE*SIZE, MPI_INT, n, 2, MPI_COMM_WORLD);

                //Send offset
                MPI_Send(&offset, 1, MPI_INT, n, 3, MPI_COMM_WORLD);
                
                //Send rows per proccess
                rows_per_proccess = cores;
                if(offset+rows_per_proccess > SIZE){
                    rows_per_proccess = SIZE-offset;
                }
                int rows[rows_per_proccess][SIZE];
                MPI_Send(&rows_per_proccess, 1, MPI_INT, n, 4, MPI_COMM_WORLD);

                //Send first matrix rows                
                k = 0;
                for(i=offset; i < (offset+rows_per_proccess); i++){
                    for(j=0; j<SIZE; j++){
                        rows[k][j] = m1[i][j];                    
                    }
                    k++;
                }

                MPI_Send(&rows, rows_per_proccess*SIZE, MPI_INT, n, 5, MPI_COMM_WORLD);

                //Receive offset
                MPI_Recv(&slave_offset, 1, MPI_INT, n, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //Receive result
                int res[rows_per_proccess][SIZE];
                MPI_Recv(&res, rows_per_proccess*SIZE, MPI_INT, n, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int f = 0;
                MPI_Send(&f, 1, MPI_INT, n, 7, MPI_COMM_WORLD);

                //Insert results into mres matrix
                for(i=0; i < rows_per_proccess; i++){
                    for(j=0; j<SIZE; j++){
                        mres[offset+i][j] = res[i][j];
                    }
                }

                offset = offset+rows_per_proccess;
            }
            if(offset >= SIZE) {
                int f = 1;                
                //Send finish signal to slaves
                for(n=1; n < nprocs; n++){
                    MPI_Send(&f, 1, MPI_INT, n, 7, MPI_COMM_WORLD);
                }
                break;
            }
        }

        // VERIFICA SE O RESULTADO DA MULTIPLICACAO ESTA CORRETO
        for (i=0 ; i<SIZE; i++) {
            k = SIZE*(i+1);
            for (j=0 ; j<SIZE; j++) {
                int k_col = k*(j+1);
                if (i % 2 ==0) {
                    if (j % 2 == 0) {
                        if (mres[i][j]!=k_col)
                        MPI_Finalize();
                        return 1;
                    }
                    else {
                        if (mres[i][j]!=-k_col)
                        MPI_Finalize();
                        return 1;
                    }
                }
                else {
                    if (j % 2 == 0) {
                        if (mres[i][j]!=-k_col)
                        MPI_Finalize();
                        return 1;
                    }
                    else {
                        if (mres[i][j]!=k_col)
                        MPI_Finalize();
                        return 1;
                    }
                }
            } 
        }

        // OBTEM O TEMPO
        elapsed_time += MPI_Wtime ();

        // MOSTRA O TEMPO DE EXECUCAO
        printf("\nelapsed_time: %lf",elapsed_time);
        printf("\n");
    }else{
        int cores, finish;
        char slave_processor_name[MPI_MAX_PROCESSOR_NAME];
        while (1){
            //Receive name
            MPI_Recv(&processor_name, MPI_MAX_PROCESSOR_NAME, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            //Set how many cores to use
            MPI_Get_processor_name(slave_processor_name, &name_len);
            if (strcmp (processor_name, slave_processor_name) == 0) {
                cores =  get_nprocs()-1;
            }else{
                cores =  get_nprocs();
            }

            MPI_Send(&cores, 1, MPI_INT, 0, 8, MPI_COMM_WORLD);

            omp_set_num_threads(cores);

            //Receive sencond matrix
            MPI_Recv(&m2, SIZE*SIZE, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            //Receive offset
            MPI_Recv(&offset, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            //Receive rows per proccess
            MPI_Recv(&rows_per_proccess, 1, MPI_INT, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            //Receive first matrix rows
            int rows[rows_per_proccess][SIZE];
            int msres[rows_per_proccess][SIZE];
            MPI_Recv(&rows, rows_per_proccess*SIZE, MPI_INT, 0, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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

            //Send msres to master
            MPI_Send(&msres, rows_per_proccess*SIZE, MPI_INT, 0, 6, MPI_COMM_WORLD);

            /* printf("\nslave - rank: %d, processor_name: %s, master_name: %s, using %d cores, offset: %d, rows_per_proccess: %d", 
                rank, 
                slave_processor_name, 
                processor_name, 
                cores,
                offset,
                rows_per_proccess  
            ); */

            //Receive finish signal
            MPI_Recv(&finish, 1, MPI_INT, 0, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(finish == 1) {
                int s = 1;
                break;
            }
        }
    }

    MPI_Finalize();
    return 0;
}

