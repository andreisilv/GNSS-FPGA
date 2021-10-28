#include <hls_stream.h>

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define I_BUS_b 64
#define TKEEP_MASK 0xFFFF
#define TSTRB_MASK 0xFFFF
#define O_BUS_b 64

// ** Input Type **
#define I_T_b 16
#define I_T_B 2                                       		// IN_T_b/8

// ** Vectors **
#define MAX_IN 4                                            // I_BUS_b/I_T_b
#define MAX_VEC 16384
#define MAX_MEM 4096                                        // MAX_VEC*(I_T_b/I_BUS_b)

// ** Operations **
#define MULT_SIZE 32                                        // 2*I_T_b
#define ACC_SIZE 43                                         // MULT_SIZE + 11 // log2(MAX_MEM)=11

// ** IO **
typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct {
	ap_fixed<O_BUS_b, ACC_SIZE> data;
	ap_uint<1> last;
} out_t;

// * Other *
#define SCALE 5

// Top-level function
void macc(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out);

// --------------------------- Test Bench  ----------------------------------

#define THRESHOLD 0.000001

#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define VEC_SIZE_M 16384
#define VEC_SIZE 16384

#define MUTED 1

int main()
{
   hls::stream<in_t> in;
   hls::stream<out_t> out;
   int err = 0;

   double out_sw = 0;

   in_t last, tmpa, tmpb;
   out_t tmpo;

   last.data = (VEC_SIZE*I_T_b + I_BUS_b - 1)/I_BUS_b - 1;
   last.strb = 0x000F; // integer
   last.keep = TKEEP_MASK;
   last.last = 1;
   in.write(last);

   /* Write data to IP input stream  */

   FILE *II = fopen("../../../../../../lib/sdrlib/test/data/II.bin", "rb");
   FILE *QQ = fopen("../../../../../../lib/sdrlib/test/data/QQ.bin", "rb");
   FILE *CP = fopen("../../../../../../lib/sdrlib/test/data/code/PROMPT.bin", "rb");
   FILE *CE = fopen("../../../../../../lib/sdrlib/test/data/code/EARLY.bin", "rb");
   FILE *CL = fopen("../../../../../../lib/sdrlib/test/data/code/LATE.bin", "rb");

   FILE *u = II;
   FILE *v = CE;

   if(u == NULL || v == NULL)
	   return -1;

   short buffer_u[MAX_IN];
   short buffer_v[MAX_IN];

   if(!MUTED) printf("U\n");
   for(int k = 0; k < VEC_SIZE; k += MAX_IN) {
	   fread(buffer_u,sizeof(short),MAX_IN,u);
	   tmpa.strb = 1;
	   for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++) {
		   ((short int*) &(tmpa.data))[i] = buffer_u[i];
		   for(int j = 0; j < I_T_B - !i; j++)
			   tmpa.strb |= tmpa.strb << 1;
		   if(!MUTED) printf("strb = %s\n", tmpa.strb.to_string().c_str());
	   }

	   if(k >= VEC_SIZE - MAX_IN)
		   tmpa.last = 1;

	   for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++)
		   if(!MUTED) printf("%d ", ((short int*) &(tmpa.data))[i]);
	   if(!MUTED) printf("\n");

	   tmpa.keep = 0xFF;
	   in.write(tmpa);
   }

   if(!MUTED) printf("V\n");
   for(int k = 0; k < VEC_SIZE; k += MAX_IN) {
	   tmpb.strb = 1;
	   fread(buffer_v,sizeof(short),MAX_IN,v);
	   for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++) {
		   ((short int*) &(tmpb.data))[i] = buffer_v[i];
		   for(int j = 0; j < I_T_B - !i; j++)
			   tmpb.strb |= tmpb.strb << 1;
	   }
	   if(k >= VEC_SIZE - MAX_IN)
		   tmpb.last = 1;

	   for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++)
		   if(!MUTED) printf("%d ", ((short int*) &(tmpb.data))[i]);
	   if(!MUTED) printf("\n");

	   in.write(tmpb);
   }

   /* HW */
   macc(in, out);
   tmpo = out.read();

   rewind(u);
   rewind(v);

   /* SW */
   int i;
   for(i = 0; i < VEC_SIZE - MAX_IN; i+= MAX_IN) {
	   fread(buffer_u,sizeof(short),MAX_IN,u);
	   fread(buffer_v,sizeof(short),MAX_IN,v);
	   for(int j = 0; j < MAX_IN; j++) {
		   out_sw += buffer_u[j]*buffer_v[j];
	   }
   }
   fread(buffer_u,sizeof(short),VEC_SIZE - i,u);
   fread(buffer_v,sizeof(short),VEC_SIZE - i,v);
   for(int j = 0; j < VEC_SIZE - i; j++) {
	   out_sw += buffer_u[j]*buffer_v[j];
   }

   out_sw *= pow(2, -SCALE);

   /* Output */
   cout << "OUTPUT HW:" << endl;
   cout << tmpo.data << endl;
   cout << "OUTPUT SW:" << endl;
   cout << out_sw << endl;

   /* Check */
   err += (tmpo.data.to_double() - out_sw) > THRESHOLD;

   /* Close */
   fclose(II);
   fclose(QQ);
   fclose(CP);
   fclose(CE);
   fclose(CL);

   return err;
}
