#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv)
{

  char string[ 100 ];
  char *ptr;

  unsigned int nr_of_intervalls = 0;

  double sum = 0.0, x = 0.0;
  double h = 0.0;


  FILE *fp = fopen( "parameters.txt", "r" );
  fgets( string, 100, fp );

  nr_of_intervalls = strtol( string, &ptr, 10);
  h = 1.0 / nr_of_intervalls;

  for ( unsigned int i = 1; i < nr_of_intervalls; i++ )
    {

      x = ( i - 1/2 ) * h;
      sum += 4 / ( 1 + x * x ); 

    }

  sum *= h;

  printf( "Summe: %10.8lf\n", sum );


  return 0;

}
