//
// Module used: env/gcc-4.9.3_openmpi-1.8.6
// Compiling: mpicc -O3 -march=native -mavx -std=c99 -lm -o midpointrule_mpi midpointrule_mpi.c
// Measurement time: job wide with "time"

//
// IO server machen 
// MPI_Allreduce und Abbruch bzgl fehlerkriterium
 
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define M_PI 3.14159265358979323

int main( int argc, char **argv )
{

  char string[ 100 ];
  char *ptr;

  unsigned int size, rank;
  unsigned long int nr_of_intervalls = 0;
  unsigned long int mflops = 0, max_mflops = 0, min_mflops = 0;
  unsigned long int increment = 0, rank_increment = 0;

  double sum = 0.0, my_sum = 0.0; 
  double x = 0.0, h = 0.0;
  double begin, end;


  MPI_Init( &argc, &argv );

  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );


  if ( rank == 0 )
    {

      FILE *fp = fopen( "parameters.txt", "r" );
      fgets( string, 100, fp );

      nr_of_intervalls = strtoul( string, &ptr, 10);
      h = 1.0 / nr_of_intervalls;

      fclose( fp );

    }

  MPI_Bcast( &nr_of_intervalls, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD );
  MPI_Bcast( &h, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD );

  begin = MPI_Wtime();

  increment = nr_of_intervalls / size; 
  rank_increment = ( rank + 1 ) * increment;

  for ( unsigned long int i = rank * increment; i < rank_increment; i++ )
    {

      x = ( i + 0.5 ) * h;
      sum += 4 / ( 1 + x * x ); 

    }

  sum *= h;

  end = MPI_Wtime();

  mflops = ( unsigned long int ) ( increment * 6 + 4 ) / ( 1000000 * ( end - begin ) );
  printf( "MFLOPS rank %i: %lu\n", rank, mflops );

  MPI_Reduce( &sum, &my_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD ); 
  MPI_Reduce( &mflops, &min_mflops, 1, MPI_LONG, MPI_MIN, 0 , MPI_COMM_WORLD );
  MPI_Reduce( &mflops, &max_mflops, 1, MPI_LONG, MPI_MAX, 0 , MPI_COMM_WORLD );

  if ( rank == 0 )
    {

      printf( "Approximation of PI: %15.13lf\n", my_sum );
      printf( "Error: %15.13g\n", fabs( my_sum - M_PI ) );
      printf( "Max. MFLOPS: %lu\n", max_mflops );
      printf( "Min. MFLOPS: %lu\n", min_mflops );
      

      FILE *fp = fopen( "output.txt", "w" );
       
      fprintf( fp, "Approximation of PI: %15.13lf\n", my_sum );
      fprintf( fp, "Error: %15.13g\n", fabs( my_sum - M_PI ) );
      fprintf( fp, "Max. MFLOPS: %lu\n", max_mflops );
      fprintf( fp, "Min. MFLOPS: %lu\n", min_mflops );
    
      fclose( fp );

    }


  MPI_Finalize( );

  return 0;

}
