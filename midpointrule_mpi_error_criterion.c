//
// Module used: env/gcc-4.9.3_openmpi-1.8.6
// Compiling: mpicc -O3 -march=native -mavx -std=c99 -lm -o midpointrule_mpi midpointrule_mpi.c
// Measurement time: job wide with "time"

//
// OK - IO server machen 
// OK - MPI_Allreduce und Abbruch bzgl fehlerkriterium
// OK - aufteilung der intervalle so, dass man nicht nur gleiche anzahl besitzt
// openmp
// - thread-binding (nur Openmp-Basis)
// OK - process binding
// kurz intel compiler

// bem:
// - float schneller als double (>= 2) schneller als long double (>= 5/4)
// - cast in code ist besser/schneller als impliiter cast 
// - output (stdout, datei) ist teuer (stdout ca. 1/3 mehr an zeit!?)
// - nutze process bindung: 
//   - map-by (round robin strategie), 
//   - bind-to (wie soll die prozessbindung statt finden 
//   - rank-by (wie soll die durchnummerierng der Prozesses gemacht werden)


#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define M_PI 3.14159265358979323

unsigned int size, rank;
unsigned long int nr_of_intervalls = 0;
unsigned long int iter = 0, nr_of_iterations = 1000000;  
unsigned long int increment = 0, rank_increment = 0;

double sum = 0.0, my_pi = 0.0, pi = 0.0, error = 10.0; 
double x = 0.0, h = 0.0;
double tolerance = 0.0;
double mflops = 0.0, max_mflops = 0.0, min_mflops = 0.0;
double begin, end;


int main( int argc, char **argv )
{

  char string[ 100 ];
  char *ptr;

  unsigned long int i = 0, j = 0;
  
  MPI_Status *status;

  
  MPI_Init( &argc, &argv );

  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );


  if ( rank == 0 )
    {

      FILE *fp = fopen( "parameters.txt", "r" );

      fgets( string, 100, fp );
      tolerance = strtod( string, &ptr );

      fgets( string, 100, fp );
      nr_of_iterations = strtoul( string, &ptr, 10 );

      fclose( fp );

    }


  MPI_Bcast( &tolerance, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD );
  MPI_Bcast( &nr_of_iterations, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD );

  begin = MPI_Wtime();

  while( ( error > tolerance ) && ( iter < nr_of_iterations ) )
    { 

      j= 0;
      sum = 0.0;

      iter = iter + 1;
      increment = iter * size; 
      h = 1.0 / ( double ) increment;

      for ( i = rank + 1; i <= increment; i += size )
	{

	  j++;
	  x = ( ( double ) i - 0.5 ) * h;
	  sum = sum + ( 4.0 / ( 1.0 + x * x ) ); 

	}

      my_pi = sum * h;
      mflops = mflops + j * 6.0 + 2.0;

      MPI_Allreduce( &my_pi, &pi, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
 
      error = fabs( pi - M_PI );

    } 
  
  end = MPI_Wtime();

  mflops = mflops / ( pow( 10, 6 ) * ( end - begin ) );
  
  MPI_Reduce( &mflops, &min_mflops, 1, MPI_DOUBLE, MPI_MIN, 0 , MPI_COMM_WORLD );
  MPI_Reduce( &mflops, &max_mflops, 1, MPI_DOUBLE, MPI_MAX, 0 , MPI_COMM_WORLD );

  if ( rank == 0 )
    {

      FILE *fp = fopen( "output.txt", "w" ); 
       
      fprintf( fp, "Approximation of PI: %15.13lf\n", pi );
      fprintf( fp, "Error: %15.13g\n", fabs( pi - M_PI ) );
      fprintf( fp, "Nr of iterations: %lu\n", iter );
      fprintf( fp, "Max. MFLOPS: %10.2g\n", max_mflops );
      fprintf( fp, "Min. MFLOPS: %10.2g\n", min_mflops );
      fprintf( fp, "Wallclocktime: %f s\n", end - begin );

      fprintf(  fp, "\n" );
      fprintf(  fp, "MFLOPS: %g\n", mflops );
        
      for ( unsigned int i = 1; i < size; i++ )
	{

	  MPI_Recv( &mflops, 1, MPI_DOUBLE, i, 99, MPI_COMM_WORLD, status );
	  fprintf(  fp, "MFLOPS: %g\n", mflops );
                    
	}

      fclose( fp );
          
    }else{

    MPI_Send( &mflops, 1, MPI_DOUBLE, 0, 99, MPI_COMM_WORLD );

  }
      
  MPI_Finalize( );

  return 0;

}
