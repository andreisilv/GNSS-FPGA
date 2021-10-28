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

#define THRESHOLD 0.196 //%

#include <stdio.h>
#include <stdlib.h>

#define VEC_SIZE_M 16384
#define VEC_SIZE 16384

#define MUTED 0

int main()
{
   hls::stream<in_t> in;
   hls::stream<out_t> out;
   int err, errl;

   in_t arg, tmpa, tmpb;
   out_t tmpo;

   FILE *signal = fopen("../../../../../../lib/sdrlib/test/data/signal.bin", "rb");
   FILE *fII = fopen("../../../../../../lib/sdrlib/test/data/II.bin", "rb");
   FILE *fQQ = fopen("../../../../../../lib/sdrlib/test/data/QQ.bin", "rb");

   char buffer[I_BUS_b/I_T_b];
   short II[O_BUS_b/O_T_b];
   short QQ[O_BUS_b/O_T_b];

   int bus_size = I_BUS_b/I_T_b;
   int i;

   if(signal == NULL)
	   return -1;

   /* Arguments */
   int last = (VEC_SIZE*I_T_b + I_BUS_b - 1)/I_BUS_b - 1;
   int dtype = REAL;
   ap_fixed<64,20> phase = 0;
   //ap_fixed<64,20> phase_step = 8.004105681984;
   ap_int<64> phase_step = 8.004105681984 * pow(2,44);
   //      <64,20>            = 8.004105681983901377
   //      <68,4>             = 0b1000.0000000100001101000100011110100111011001010111000000000000000000

   /*
   printf("phase step = %s\n", phase_step.to_string().c_str());
   float phases = 8.004105681984;
   float phased = 0;
   ap_fixed<64,20> phasef;
   FILE *stxt = fopen("../../../../../../../../rascunhos/S.txt", "w");
   for(int i = 0; i < VEC_SIZE/2; i++) {
	   phased += 2*phases;
	   phasef = phased;
	   fprintf(stxt, "%f\n", phasef.to_float());
   }
   fclose(stxt);
    */

   arg.keep = TKEEP_MASK;
   arg.strb = TSTRB_MASK;
   arg.last = 1;

   // last

   arg.data = last;
   in.write(arg);

   // data type

   arg.data = dtype;
   in.write(arg);

   // phase / phase step (of width 64, written in two steps; in order to increase precision)

   arg.data = phase.range(31,0);
   in.write(arg);
   arg.data = phase.range(63,32);
   in.write(arg);

   arg.data = phase_step.range(31,0);
   in.write(arg);
   arg.data = phase_step.range(63,32);
   in.write(arg);

   /* Write Data  */

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

   /* Check */
   err = 0, errl = err;
   bus_size = O_BUS_b/O_T_b;

   if(!MUTED)
	   printf("SW.I : HW.I \t SW.Q : HW.Q\n");

   for(int i = 0; i < 2*VEC_SIZE; i+= bus_size) {

	   tmpo = out.read();
	   fread(II, sizeof(char), bus_size, fII);
	   fread(QQ, sizeof(char), bus_size, fQQ);

	   for(int j = 0; j < bus_size/2; j ++) {
		   if((II[j] != ((short*) &tmpo)[j]) || (QQ[j] - ((short*) &tmpo)[bus_size/2+j]))
			   err ++;

		   if(!MUTED && (err != errl)) {
			   printf("%+3d : %+3d\t%+3d : %+3d #%6d (Word %5d) [#Error=%3d]\n", II[j], ((short*) &tmpo)[j], QQ[j], ((short*) &tmpo)[bus_size/2+j], i, i/bus_size, err);
			   errl = err;
		   }
	   }
   }

   /* Close */
   fclose(signal);
   fclose(fII);
   fclose(fQQ);

   return err > (THRESHOLD/100)*VEC_SIZE;
}
