/*
 * aoat.c - Matrix aoat: grand-sum(A o A^T).
 *
 * Each aoat function must have a prototype of the form:
 * int aoat (int N, const int A[N][N]);
 *
 * A aoat function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

/*
 * aoat_submit - This is the solution aoat function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Aoat submission", as the driver
 *     searches for that string to identify the aoat function to
 *     be graded.
 */
char aoat_submit_desc[] = "Aoat submission";
int aoat_submit (int N, const int A[N][N]) {
  int row_block, col_block;
  int row, col;
  int sum = 0;

/* 
Specify exact matrix size of 32 X 32, go through and block each row/column
by 8 and then go through those blocks performing their arithmetic values into sum
since this is an N X N matrix we can double our work when the row > col.
*/
  if (N == 32) {
  	for (row_block = 0; row_block < N; row_block += 8) {
	 	 for (col_block = 0; col_block < N; col_block += 8) {
			  for (row = row_block; row < N && row < row_block + 8; row++) {
				  for (col = col_block; col < N && col < col_block + 8; col++) {
					  if (row == col) {
						  sum += A[row][col] * A[col][row];
					  } else if (row > col) {
						  sum += 2 * (A[row][col] * A[col][row]); 
            }
          }
        }
      }
    }
/* 
Specify exact matrix size of 64 X 64, go through and block each row/column
by 4 and then go through those blocks performing their arithmetic values into sum
since this is an N X N matrix we can double our work when the row > col.
*/
  } else if (N == 64) {
  	for (row_block = 0; row_block < N; row_block += 4) {
      for (col_block = 0; col_block < N; col_block += 4) {
        for (row = row_block; row < N && row < row_block + 4; row++) {
          for (col = col_block; col < N && col < col_block + 4; col++) {
            if (row == col) {
              sum += A[row][col] * A[col][row];
            }
            else if (row > col) {
                 sum += 2 * (A[row][col] * A[col][row]);
            }
          }
		    }
      }
    }
/* 
Specify exact matrix size of 67 X 67, go through and block each row/column
by 16 and then go through those blocks performing their arithmetic values into sum
since this is an N X N matrix we can double our work when the row > col.
*/
  } else if (N == 67) {
	for (row_block = 0; row_block < N; row_block += 16) {
     for (col_block = 0; col_block < N; col_block += 16) {
       for (row = row_block; row < N && row < row_block + 16; row++) {
         for (col = col_block; col < N && col < col_block + 16; col++) {
           if (row == col) {
             sum += A[row][col] * A[col][row];
            }
            else if (row > col) {
              sum += 2 * (A[row][col] * A[col][row]);
            }
          }
        }
      }
    }
  }
  return sum;
}


/*
 * You can define additional aoat functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * aoat - A simple baseline aoat function, not optimized for the cache.
 */
char aoat_desc[] = "Simple row-wise scan aoat";
int aoat (int N, const int A[N][N]) {
  int i, j, res = 0;

  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {

      res += A[j][i] * A[i][j];
    }
  }

  return res;
}

/*
 * registerAoatFunctions - This function registers your aoat
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     aoat strategies.
 */
void registerAoatFunctions() {
  /* Register your solution function */
  registerAoatFunction (aoat_submit, aoat_submit_desc);

  /* Register any additional aoat functions */
  registerAoatFunction (aoat, aoat_desc);
}
