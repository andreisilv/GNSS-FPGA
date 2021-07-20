#include <ap_int.h>

#include <hls_stream.h>
#include <ap_axi_sdata.h>

// ** LOGICAL SIZES **

// IO Type
#define I_T_b				8
#define O_T_b				16		// > ADD_BUS_b

// IO Type . auxiliar
#define LOG_I_T_b 3
#define I_T_B 2
#define I_T_B_MASK 0x3
#define x2_I_T_B_MASK 0xF
#define x2_I_T_b 16
#define x2_I_T_B 4
#define x2_O_T_b 32
#define x4_O_T_b 64

// Local Carrier Table

#define COST_ENTRIES	7
#define COST_ENTRIES_b	8
#define INDEX_b			5
#define INDEX_MASK 		0x1F
#define D90				8

// ** Other constants **
#define COMPLEX 1
#define REAL 0

// ** BUS SIZES **
#define I_BUS_b				32
#define O_BUS_b				128
#define MUL_BUS_b			10 		// most GNSS receivers A/DC quantize 4 bits (signed) + trig vector 5 bits (unsigned), 6 bits (signed)
#define ADD_BUS_b			11		// MUL_BUS_b + 1 = 11

#define LOG_I_BUS_b 5
#define LOG_O_BUS_b 8

#define TKEEP_MASK 0xFFFF
#define TSTRB_MASK 0xFFFF

// ** IO **
#define LENGTH_REGISTER_b 20

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

const float pi = 3.1415926535897932384626;

// --------------------------- IP  ----------------------------------

void mixcarr(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out);

// --------------------------- Test Bench  ----------------------------------

#define THRESHOLD 0.000001

#include <stdio.h>
#include <stdlib.h>

#define VEC_SIZE_M 16384
#define VEC_SIZE 16384

#define MUTED 0

void mixcarrsw(FILE* signal, short* II, short *QQ, ap_fixed<32,16> phase, ap_fixed<32,16> phase_step, int dtype);

int main()
{
   hls::stream<in_t> in;
   hls::stream<out_t> out;
   int err;

   in_t arg, tmpa, tmpb;
   out_t tmpo;

   FILE *signal = fopen("../../../../../../lib/sdrlib/test/IF_GN3S_G03/signal.bin", "rb");
   short II[VEC_SIZE];
   short QQ[VEC_SIZE];

   if(signal == NULL)
	   return -1;

   /* Arguments */
   int last = (VEC_SIZE*I_T_b + I_BUS_b - 1)/I_BUS_b - 1;
   int dtype = REAL;
   ap_fixed<32,16> phase = 0;
   ap_fixed<32,16> phase_step = 8.004105681984f;

   arg.keep = TKEEP_MASK;
   arg.strb = TSTRB_MASK;
   arg.last = 1;

   // last
   arg.data = last;
   in.write(arg);

   // data type
   arg.data = dtype;
   in.write(arg);

   // phase / phase step
   arg.data = phase;
   in.write(arg);
   arg.data = phase_step;
   in.write(arg);

   /* Write Data  */

   char buffer[I_BUS_b/I_T_b];

   int bus_size = I_BUS_b/I_T_b;
   int i;

   for(i = 0; i < VEC_SIZE; i += bus_size) {

	   fread(buffer, sizeof(char), bus_size, signal);

	   tmpa.strb = 1;
	   for(int j = 0; j < bus_size; j ++) {
		   ((char*) &(tmpa.data))[j] = buffer[j];
		   for(int k = 0; k < I_T_B - !j; k++)
			   tmpa.strb |= tmpa.strb << 1;
	   }

	   if(i >= VEC_SIZE - bus_size)
		   tmpa.last = 1;

	   tmpa.keep = TKEEP_MASK;

	   in.write(tmpa);
   }

   /* HW */
   mixcarr(in, out);

   /* SW */
   mixcarrsw(signal, II, QQ, phase, phase_step, dtype);

   err = 0;
   bus_size = O_BUS_b/O_T_b;

   if(!MUTED) printf("SW.I = HW.I && SW.Q == HW.Q\n");
   for(int i = 0; i < 2*VEC_SIZE; i+= bus_size) {
	   tmpo = out.read();
	   printf("> out %d\n", i/bus_size);
	   for(int j = 0; j < bus_size/2; j ++) {
		   err += II[i/2 + j] - ((short*) &tmpo)[j];
		   err += QQ[i/2 + j] - ((short*) &tmpo)[bus_size/2+j];
		   if(!MUTED && err) printf("%+3d == %+3d\t%+3d == %+3d\n", II[i/2 + j], ((short*) &tmpo)[j], QQ[i/2 + j], ((short*) &tmpo)[bus_size/2+j]);
	   }
   }

   /* Close */
   fclose(signal);

   return err;
}

void mixcarrsw(FILE* signal, short* II, short *QQ, ap_fixed<32,16> phase, ap_fixed<32,16> phase_step, int dtype)
{
	rewind(signal);

	ap_fixed<64,32> ph = phase;
	char buffer[2];
	short mul1, mul2, mul3, mul4, cost[32], sint[32];
	int index;

	// local table
	for (int i = 0; i < 32; i++) {
		cost[i] = (short) floor(32*cos((pi/16)*i)+0.5);
		sint[i] = (short) floor(32*sin((pi/16)*i)+0.5);
	}

	for(int i = 0; i < VEC_SIZE; i+= 2)
	{
		/* complex case:
	     *
		 *  II = cos*data.r - sin*data.i;
         *  QQ = sin*data.r + cos*data.i;
		 *
		 */

		fread(buffer, sizeof(char), 2, signal);

		index = ((int) ph) & INDEX_MASK;

		mul1 = cost[index]*buffer[0];
		mul2 = sint[index]*buffer[0];

		if(dtype == REAL) {
			ph += phase_step;
			index = ((int) ph) & INDEX_MASK;
		}

		mul3 = cost[index]*buffer[1];
		mul4 = sint[index]*buffer[1];

		if(dtype == COMPLEX) {
			II[i] = mul1 - mul4;
			QQ[i] = mul2 - mul3;
		} else {
			II[i] = mul1;
			QQ[i] = mul2;
			II[i + 1] = mul3;
			QQ[i + 1] = mul4;
		}

		ph += phase_step;
	}
}
