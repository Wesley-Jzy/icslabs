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
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k;
    int tmp_1, tmp_2, tmp_3, tmp_4,
    	tmp_5, tmp_6, tmp_7, tmp_8;

    /* 32X32 */
	if (M == 32 && N == 32)
	{
		for (j = 0; j < 32; j += 8) 
		{
        	for (i = 0; i < 32; i++) 
        	{
	            tmp_1 = A[i][j];
	            tmp_2 = A[i][j+1];
	            tmp_3 = A[i][j+2];
	            tmp_4 = A[i][j+3];
	            tmp_5 = A[i][j+4];
	            tmp_6 = A[i][j+5];
	            tmp_7 = A[i][j+6];
	            tmp_8 = A[i][j+7];
	            B[j][i] = tmp_1;
	            B[j+1][i] = tmp_2;
	            B[j+2][i] = tmp_3;
	            B[j+3][i] = tmp_4;
	            B[j+4][i] = tmp_5;
	            B[j+5][i] = tmp_6;
	            B[j+6][i] = tmp_7;
	            B[j+7][i] = tmp_8;
	        }
	    }
    }

    if (M == 64 && N == 64)
    {
    	for (j = 0; j < 64; j += 8) 
	    {
	        for (i = 0; i < 64; i+= 8) 
	        {
	        	for (k = i; k < i + 4; k++)
	        	{
	        		tmp_1 = A[k][j];
	        		tmp_2 = A[k][j+1];
	        		tmp_3 = A[k][j+2];
	        		tmp_4 = A[k][j+3];
	        		tmp_5 = A[k][j+4];
	        		tmp_6 = A[k][j+5];
	        		tmp_7 = A[k][j+6];
	        		tmp_8 = A[k][j+7];

	        		B[j][k] = tmp_1;
	        		B[j+1][k] = tmp_2;
	        		B[j+2][k] = tmp_3;
	        		B[j+3][k] = tmp_4;

	        		B[j][k+4] = tmp_5;
	        		B[j+1][k+4] = tmp_6;
	        		B[j+2][k+4] = tmp_7;
	        		B[j+3][k+4] = tmp_8;
	        	}

	        	for (k = j; k < j + 4; k++)
	        	{
	        		tmp_1 = B[k][i+4];
	        		tmp_2 = B[k][i+5];
	        		tmp_3 = B[k][i+6];
	        		tmp_4 = B[k][i+7];

	        		tmp_5 = A[i+4][k];
	        		tmp_6 = A[i+5][k];
	        		tmp_7 = A[i+6][k];
	        		tmp_8 = A[i+7][k];

	        		B[k][i+4] = tmp_5;
	        		B[k][i+5] = tmp_6;
	        		B[k][i+6] = tmp_7;
	        		B[k][i+7] = tmp_8;

	        		tmp_5 = A[i+4][k+4];
	        		tmp_6 = A[i+5][k+4];
	        		tmp_7 = A[i+6][k+4];
	        		tmp_8 = A[i+7][k+4];

	        		B[k+4][i] = tmp_1;
	        		B[k+4][i+1] = tmp_2;
	        		B[k+4][i+2] = tmp_3;
	        		B[k+4][i+3] = tmp_4;
	        		B[k+4][i+4] = tmp_5;
	        		B[k+4][i+5] = tmp_6;
	        		B[k+4][i+6] = tmp_7;
	        		B[k+4][i+7] = tmp_8;
	        	}
	        }
	    } 
    }

    if (M == 61 && N == 67)
    {
    	for (j = 0; j < 61; j += 8) 
	    {
	        for (i = 0; i < 67; i++) 
	        {
	            tmp_1 = A[i][j];
	            tmp_2 = A[i][j+1];
	            tmp_3 = A[i][j+2];
	            tmp_4 = A[i][j+3];
	            tmp_5 = A[i][j+4];
	            tmp_6 = A[i][j+5];
	            tmp_7 = A[i][j+6];
	            tmp_8 = A[i][j+7];
	            B[j][i] = tmp_1;
	            B[j+1][i] = tmp_2;
	            B[j+2][i] = tmp_3;
	            B[j+3][i] = tmp_4;
	            B[j+4][i] = tmp_5;
	            B[j+5][i] = tmp_6;
	            B[j+6][i] = tmp_7;
	            B[j+7][i] = tmp_8;
	        }
	    }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) 
    {
        for (j = 0; j < M; j++) 
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}


/* 
 * trans for 32X32.
 * In this case, there're 32 sets
 * And each set can store 8 int
 * So, we need to tackle 8 int in one iter 
 * (Because 12 vars is the limit)
 */
char trans_32_desc[] = "32X32";
void trans_32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int tmp_1, tmp_2, tmp_3, tmp_4,
    	tmp_5, tmp_6, tmp_7, tmp_8;

    for (j = 0; j < 32; j += 8) 
    {
        for (i = 0; i < 32; i++) 
        {
            tmp_1 = A[i][j];
            tmp_2 = A[i][j+1];
            tmp_3 = A[i][j+2];
            tmp_4 = A[i][j+3];
            tmp_5 = A[i][j+4];
            tmp_6 = A[i][j+5];
            tmp_7 = A[i][j+6];
            tmp_8 = A[i][j+7];
            B[j][i] = tmp_1;
            B[j+1][i] = tmp_2;
            B[j+2][i] = tmp_3;
            B[j+3][i] = tmp_4;
            B[j+4][i] = tmp_5;
            B[j+5][i] = tmp_6;
            B[j+6][i] = tmp_7;
            B[j+7][i] = tmp_8;
        }
    }    
}

/* 
 * trans for 64X64.
 * In this case, there're 32 sets
 * And each set can store 8 int
 * We cut it into 8 * 8 blocks
 * each block B[][], we do following chang
 * A[][]: |A B|
 *        |C D|
 * B[][]: |   |___|A'B'|___|A'C'|___|A'C'|
 * 	      |   |   |    |   |B'  |   |B'D'| 
 * (Because 12 vars is the limit)
 */
char trans_64_desc[] = "64X64";
void trans_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    int tmp_1, tmp_2, tmp_3, tmp_4,
    	tmp_5, tmp_6, tmp_7, tmp_8;

    for (j = 0; j < 64; j += 8) 
    {
        for (i = 0; i < 64; i+= 8) 
        {
        	for (k = i; k < i + 4; k++)
        	{
        		tmp_1 = A[k][j];
        		tmp_2 = A[k][j+1];
        		tmp_3 = A[k][j+2];
        		tmp_4 = A[k][j+3];
        		tmp_5 = A[k][j+4];
        		tmp_6 = A[k][j+5];
        		tmp_7 = A[k][j+6];
        		tmp_8 = A[k][j+7];

        		B[j][k] = tmp_1;
        		B[j+1][k] = tmp_2;
        		B[j+2][k] = tmp_3;
        		B[j+3][k] = tmp_4;

        		B[j][k+4] = tmp_5;
        		B[j+1][k+4] = tmp_6;
        		B[j+2][k+4] = tmp_7;
        		B[j+3][k+4] = tmp_8;
        	}

        	for (k = j; k < j + 4; k++)
        	{
        		tmp_1 = B[k][i+4];
        		tmp_2 = B[k][i+5];
        		tmp_3 = B[k][i+6];
        		tmp_4 = B[k][i+7];

        		tmp_5 = A[i+4][k];
        		tmp_6 = A[i+5][k];
        		tmp_7 = A[i+6][k];
        		tmp_8 = A[i+7][k];

        		B[k][i+4] = tmp_5;
        		B[k][i+5] = tmp_6;
        		B[k][i+6] = tmp_7;
        		B[k][i+7] = tmp_8;

        		tmp_5 = A[i+4][k+4];
        		tmp_6 = A[i+5][k+4];
        		tmp_7 = A[i+6][k+4];
        		tmp_8 = A[i+7][k+4];

        		B[k+4][i] = tmp_1;
        		B[k+4][i+1] = tmp_2;
        		B[k+4][i+2] = tmp_3;
        		B[k+4][i+3] = tmp_4;
        		B[k+4][i+4] = tmp_5;
        		B[k+4][i+5] = tmp_6;
        		B[k+4][i+6] = tmp_7;
        		B[k+4][i+7] = tmp_8;
        	}
        }
    }    
}

char trans_61_desc[] = "61X67";
void trans_61(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int tmp_1, tmp_2, tmp_3, tmp_4,
    	tmp_5, tmp_6, tmp_7, tmp_8;

    for (j = 0; j < 61; j += 8) 
    {
        for (i = 0; i < 67; i++) 
        {
            tmp_1 = A[i][j];
            tmp_2 = A[i][j+1];
            tmp_3 = A[i][j+2];
            tmp_4 = A[i][j+3];
            tmp_5 = A[i][j+4];
            tmp_6 = A[i][j+5];
            tmp_7 = A[i][j+6];
            tmp_8 = A[i][j+7];
            B[j][i] = tmp_1;
            B[j+1][i] = tmp_2;
            B[j+2][i] = tmp_3;
            B[j+3][i] = tmp_4;
            B[j+4][i] = tmp_5;
            B[j+5][i] = tmp_6;
            B[j+6][i] = tmp_7;
            B[j+7][i] = tmp_8;
        }
    }    
}

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

    registerTransFunction(trans_32, trans_32_desc); 

    registerTransFunction(trans_64, trans_64_desc); 

    registerTransFunction(trans_61, trans_61_desc); 
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

