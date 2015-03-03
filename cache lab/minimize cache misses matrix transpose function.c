/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */


// This function checks if B is the transpose of A
int is_transpose(int M, int N, int A[N][M], int B[M][N]){
    int i, j;

    for (i = 0; i < N; i++){
        for(j = 0; j < M; ++j){
	    if (A[i][j] != B[j][i]) {
	        return 0;
	    }
	}
    }

    return 1;
}

// A simple transpose function; 
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]){
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

char dNothing_desc[] = "A function does nothing";
void dNothing(int M, int N, int A[N][M], int B[M][N]){
    (void)A;
    (void)B;
    return;
}

// Please fill in your solution here
// This function is evaluated by autolab to determine your score 
// for part (b)
// Please do not change this description
char transpose_submit_desc[] = "Part (b) Submit";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]){
    int n, sn, m, sm, i, j; 
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    switch(N){
	case 32:
	for (n = 0; n < 25; n += 8){
		for(m = 0; m < 25; m += 8 ){
			if(m != n){
				for(i = n; i < n+8; i++){
					for(j =m; j < m+8; j++){
						B[j][i] = A[i][j];
					}
				}
			}
		}
	}
	for (n = 0; n < 25; n += 8){
		for(i = n; i < n+8; i++){
			for(j = n; j < n+8; j++){
				if(i != j){
				 B[j][i] = A[i][j];
				}
			}
			B[i][i] = A[i][i]; 
		}			
	}
	break;
	case 64:
	for(n = 0; n < 57; n +=8){
		for(m = 0; m < 57; m += 8){
		    if(m != n){
		    	sn = n;
		    	for(sm = m; sm < m + 5; sm += 4){
				for(i = sn; i < sn+4; i++){
			  		for(j = sm; j < sm+4; j++){
						B[j][i] = A[i][j];
			  		}
				}
		   	} 
		   	sn = n+4;
		   	for(sm = m + 4; sm > m-1; sm -=4){
				for(i = sn; i < sn+4; i++){
   			  		for(j = sm; j < sm+4; j++){
						B[j][i] = A[i][j];
					}		
				}	
		   	}
		  }
		}	
	}
	for(n = 0; n < 57; n+=8){
		sn = n;
		for(i = sn; i < sn+4; i++){
			for(j = sn; j < sn+4; j++){
				if(i != j){
					B[j][i] = A[i][j];
				}
			}
			B[i][i] = A[i][i];
		}

		for(i = sn; i < sn+4; i++){
			for(j = sn+4; j < sn+8; j++){
				B[j][i] = A[i][j];
			}
		}
		sn = n+4;
		for(i = sn; i < sn+4; i++){
   			for(j = sn; j < sn+4; j++){
				if(i != j){
					B[j][i] = A[i][j];
				}
			}
			B[i][i] = A[i][i];
		}

		for(i = sn; i < sn+4; i++){
	   		for(j = sn-4; j < sn; j++){
				B[j][i] = A[i][j];
			}
		}		
	}	
	break;
	case 67:
	for(m = 0; m < 61; m += 16){
		for(n = 0; n < 67; n += 20){
			for(i = n; i < n + 20 && i < 67; i++){
				for(j = m; j < m + 16 && j < 61; j++){
					B[j][i] = A[i][j];
				}
			}
		}
	}
	
    
	break;
}
    

    ENSURES(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */



/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}



