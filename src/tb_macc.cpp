#include <hls_stream.h>

#include <ap_int.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define IN_BUS_SIZE  128
#define OUT_BUS_SIZE 64

// ** Input Type **
#define IN_T_BITS 16
#define IN_T_BYTES 2                                        // IN_T_BITS/8

// ** IO **
typedef struct ap_axis<IN_BUS_SIZE, 0, 0, 0> in_t;
typedef struct ap_axis<OUT_BUS_SIZE, 0, 0, 0> out_t;

// ** Vetores **
#define MAX_IN 8                                            // IN_BUS_SIZE/IN_T_BITS
#define MAX_VEC 16384
#define MAX_MEM 2048                                        // MAX_VEC*(IN_T_BITS/IN_BUS_SIZE)

// ** Operaçõees **
#define MULT_SIZE 32                                        // 2*IN_T_BITS
#define ACC_SIZE 43                                         // MULT_SIZE + 11 // log2(MAX_MEM)

#include <stdio.h>
#include <stdlib.h>

#define VEC_SIZE 		16

const short int u[VEC_SIZE] = {-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0};
const short int v[VEC_SIZE] = { 1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0};

static ap_int<OUT_BUS_SIZE> hw_dp = 0;
static ap_int<OUT_BUS_SIZE> sw_dp = 0;

// Top-level function
void macc(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out);

int main()
{
   hls::stream<in_t> in;
   hls::stream<out_t> out;
   int err = 0;

   in_t tmpa, tmpb; out_t tmpo;

   /* Escrever vetores na stream de entrada do IP  */
   printf("U\n");
   for(int k = 0; k < VEC_SIZE; k += MAX_IN) {
	   tmpa.keep = 1;
	   for(int i = 0; i < MAX_IN && i < VEC_SIZE; i++) {
		   ((short int*) &(tmpa.data))[i] = u[k + i];
		   for(int j = 0; j < IN_T_BYTES - !i; j++)
			   tmpa.keep |= tmpa.keep << 1;
	   }
	   if(k >= VEC_SIZE - MAX_IN)
		   tmpa.last = 1;

	   for(int i = 0; i < MAX_IN && i < VEC_SIZE; i++)
		   printf("%d ", ((short int*) &(tmpa.data))[i]);
	   printf("\n");

	   in.write(tmpa);
   }

   printf("V\n");
   for(int k = 0; k < VEC_SIZE; k += MAX_IN) {
	   tmpb.keep = 1;
	   for(int i = 0; i < MAX_IN && i < VEC_SIZE; i++) {
		   ((short int*) &(tmpb.data))[i] = v[k + i];
		   for(int j = 0; j < IN_T_BYTES - !i; j++)
			   tmpb.keep |= tmpb.keep << 1;
	   }
	   if(k >= VEC_SIZE - MAX_IN)
		   tmpb.last = 1;

	   for(int i = 0; i < MAX_IN && i < VEC_SIZE; i++)
		   printf("%d ", ((short int*) &(tmpb.data))[i]);
	   printf("\n");

	   in.write(tmpb);
   }

   /* HW */
   macc(in, out);
   tmpo = out.read();

   /* SW */
   for(int i = 0; i < VEC_SIZE; i++)
	   sw_dp += u[i]*v[i];

   /* Check */
   hw_dp = tmpo.data;
   printf("%d == %d ?\n", (int) hw_dp, (int) sw_dp);
   err += (hw_dp - sw_dp) != 0;

   return err;
}
