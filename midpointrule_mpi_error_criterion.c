//
// Module used: env/gcc-4.9.3_openmpi-1.8.6
// Compiling: mpicc -O3 -march=native -mavx -std=c99 -lm -o midpointrule_mpi midpointrule_mpi.c
// Measurement time: job wide with "time"

//
// OK - IO server machen 
// OK - MPI_Allreduce und Abbruch bzgl fehlerkriterium
// OK - aufteilung der intervalle so, dass man nicht nur gleiche anzahl besitzt
// - openmp
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


//
// Interessante Beobachtung auf Laptop: (8 GB, 1 Kern, virtuele maschine linux)
// -1 Prozess: ca. 11 sek
// - 2 Prozesse: ca. 6 sek
// 4 Prozesse: ca. 3.5 sek
// 8 Prozesse: ca. 2 sek
// 16 Prozesse: ca. 1.5 sek
// 24 Prozesse: ca. 2 sek
// 32 Prozesse: ca. 1.5 sek
// -- eigentlich sollte das spaetestens bei >= 2 Prozessen wieder nach oben gehen (overcommitting)


#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M_PI 3.14159265358979323


int size, rank, rank_id_max, rank_id_min;
unsigned long int nr_of_intervalls = 0;
unsigned long int iter = 0, nr_of_iterations = 1000000;  
unsigned long int min_iterations = 0, max_iterations = 0;
unsigned long int tmp_min_iter = 0, tmp_max_iter = 0;

double sum = 0.0, my_pi = 0.0, pi = 0.0, error = 10.0; 
double x = 0.0, h = 0.0;
double tolerance = 0.0;
double mflops = 0.0, max_mflops = 0.0, min_mflops = 0.0;
double tmp_max_mflops = 0.0, tmp_min_mflops = 0.0;
double begin, end;


struct data{
  char hostname[ 100 ];
  unsigned long int iterations;
  double mflops;
};

struct data instance, instance_tmp;


int main( int argc, char **argv )
{

  char string_tmp[ 100 ], processor_name[ 100 ];

  int processor_name_length = 0;
  unsigned long int i = 0, j = 0;
  
  MPI_Status *status = NULL;

  
  MPI_Init( &argc, &argv );

  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  int blocklengths[ 3 ] = { 100, 1, 1 };
  MPI_Aint offsets[ 3 ];
  MPI_Datatype oldtypes[ 3 ] = { MPI_CHAR, MPI_UNSIGNED_LONG, MPI_DOUBLE };
  MPI_Datatype structtype;
  
  MPI_Get_address( &instance.hostname, &offsets[ 0 ] );
  MPI_Get_address( &instance.iterations, &offsets[ 1 ] );
  MPI_Get_address( &instance.mflops, &offsets[ 2 ] );

  for( int j = 2; j >= 0; j-- )
	  offsets[ j ] -= offsets[ 0 ]; 
  
  MPI_Type_create_struct( 3, blocklengths, offsets, oldtypes, &structtype );
  MPI_Type_commit( &structtype );

  if ( rank == 0 )
    {

      FILE *fp = fopen( "parameters.txt", "r" );

      fgets( string_tmp, 100, fp );
      tolerance = strtod( string_tmp, NULL );

      fgets( string_tmp, 100, fp );
      nr_of_iterations = strtoul( string_tmp, NULL, 10 );

      fclose( fp );

      printf( "tolerance = %15.13f, nr_of_iterations = %lu\n", tolerance, nr_of_iterations );

    }


  MPI_Bcast( &tolerance, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD );
  MPI_Bcast( &nr_of_iterations, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD );

  begin = MPI_Wtime();

  while( ( error > tolerance ) && ( iter < nr_of_iterations ) )
    { 

      j = 0;
      sum = 0.0;

      iter = iter + 1;
      nr_of_intervalls = iter * size; 
      h = 1.0 / ( double ) nr_of_intervalls;

      for ( i = rank + 1; i <= nr_of_intervalls; i += size )
	{

	  j++;
	  x = ( ( double ) i - 0.5 ) * h;
	  sum = sum + ( 4.0 / ( 1.0 + x * x ) ); 

	}

      my_pi = sum * h;
      mflops = mflops + ( j + 1 ) * 6.0;

      MPI_Allreduce( &my_pi, &pi, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
 
      error = fabs( pi - M_PI );

    } 
  
  end = MPI_Wtime();

  instance.iterations = iter;
  instance.mflops = mflops / ( pow( 10, 6 ) * ( end - begin ) );
  MPI_Get_processor_name( processor_name, &processor_name_length );
  strncpy( instance.hostname, processor_name, sizeof( instance.hostname ) );
    
  if ( rank == 0 )
    {

      struct data array[ size ];
        
      array[ 0 ].mflops = instance.mflops;
      array[ 0 ].iterations = instance.iterations;
      strncpy( array[ 0 ].hostname, instance.hostname, sizeof( array[ 0 ].hostname ) );
      
      for ( unsigned int i = 1; i < size; i++ )
	{

	  MPI_Recv( &instance_tmp, 1, structtype, i, 99, MPI_COMM_WORLD, status );
	  array[ i ].mflops = instance_tmp.mflops; 
	  array[ i ].iterations = instance_tmp.iterations; 
	  strncpy( array[ i ].hostname, instance_tmp.hostname, sizeof( array[ i ].hostname ) ); 
      
	}

      //
      // Comparison and output to file
	
      FILE *fp = fopen( "output.txt", "w" ); 
       
      fprintf( fp, "Approximation of PI: %15.13lf\n", pi );
      fprintf( fp, "Error: %15.13g\n", fabs( pi - M_PI ) );
      fprintf( fp, "\n" );
      fprintf( fp, "Wallclocktime of process 0: %f s\n", end - begin );
      fprintf( fp, "\n" );
          
     
      //
      // Iterations

      tmp_max_iter = array[ 0 ].iterations;
      tmp_min_iter = array[ 0 ].iterations;

      rank_id_max = 0;
      rank_id_min = 0;
    
      for( unsigned int i = 1; i < size; i++ )
	{
          
      	  if ( array[ i ].iterations > tmp_max_iter )
	    {
              
              tmp_max_iter = array[ i ].iterations;
              rank_id_max = i;
              
	    }

	  if ( array[ i ].iterations < tmp_min_iter )
	    {
              
              tmp_min_iter = array[ i ].iterations;
              rank_id_min = i;
                   
	    }
          
	}
      
      
      fprintf( fp, "Max. iterations of process %i (node %s): %lu\n", rank_id_max, array[ rank_id_max ].hostname, tmp_max_iter );
      fprintf( fp, "Min. iterations of process %i (node %s): %lu\n", rank_id_min, array[ rank_id_min ].hostname, tmp_min_iter );
      fprintf( fp, "\n" );

      for ( unsigned int i = 0; i < size; i++ )
        fprintf( fp, "Iterations of process %i: %lu\n", i, array[ i ].iterations );
      fprintf( fp, "\n\n" );

      //
      // MFLOPS

      tmp_max_mflops = array[ 0 ].mflops;
      tmp_min_mflops = array[ 0 ].mflops;

      rank_id_max = 0;
      rank_id_min = 0;

      tmp_max_mflops = array[ 0 ].mflops;
      tmp_min_mflops = array[ 0 ].mflops;
  
      for ( unsigned int i = 1; i < size; i++ )
	{

	  if ( array[ i ].mflops > tmp_max_mflops  )
	    {
          
	      tmp_max_mflops = array[ i ].mflops;
	      rank_id_max = i;
          
	    }

	  if ( array[ i ].mflops < tmp_min_mflops )
	    {
          
	      tmp_min_mflops = array[ i ].mflops;
	      rank_id_min = i;
          
	    }
                   
	}

      fprintf( fp, "Max. MFLOPS of process %i (node %s): %g\n", rank_id_max, array[ rank_id_max ].hostname, tmp_max_mflops );
      fprintf( fp, "Min. MFLOPS of process %i (node %s): %g\n", rank_id_min, array[ rank_id_min ].hostname, tmp_min_mflops );
      fprintf( fp, "\n" );

      for ( unsigned int i = 0; i < size; i++ )
        fprintf( fp, "MFLOPS of process %i: %g\n", i, array[ i ].mflops );
	  

      fclose( fp );
          
    }else{

    MPI_Send( &instance, 1, structtype, 0, 99, MPI_COMM_WORLD );    

  }
      
  MPI_Type_free( &structtype );
      
  MPI_Finalize( );

  return 0;

}
