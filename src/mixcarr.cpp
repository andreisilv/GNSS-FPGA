#include <ap_int.h>

#include <hls_stream.h>
#include <ap_axi_sdata.h>

/* LOGICAL SIZES */
#define I_MAX				16384
#define I_SIZE_b			8
#define O_SIZE_b			32
#define MEM_ENTRIES			32
#define MEM_SIZE_b			16

/* LOCAL MEM */
const short int cost[MEM_SIZE_vec] = {32, 31, 30, 27, 23, 18, 12, 6, 0, -6, -12, -18, -23, -27, -30, -31, -32, -31, -30, -27, -23, -18, -12, -6, 0, 6, 12, 18, 23, 27, 30, 31};
const short int sint[MEM_SIZE_vec] = {0, 6, 12, 18, 23, 27, 30, 31, 32, 31, 30, 27, 23, 18, 12, 6, 0, -6, -12, -18, -23, -27, -30, -31, -32, -31, -30, -27, -23, -18, -12, -6};
const float pi = 3.1415926535897932384626;

/* BUS SIZES */
#define I_BUS_b				128
#define O_BUS_b				128
#define MUL_BUS_b			I_SIZE_b + MEM_SIZE_b
#define ADD_BUS_b			MUL_BUS_b + 1

// ** IO **
typedef struct ap_axis<IN_BUS_SIZE, 0, 0, 0> in_t;
typedef struct ap_axis<OUT_BUS_SIZE, 0, 0, 0> out_t;

/*
	II = cost[index] * p[0] - sint[index] * p[1]
   	QQ = sint[index] * p[0] + cost[index] * p[1]
*/

void mixcarr(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out, ap_inte<1> *dtype, float *freq, float *ti, float *phi0)
{
	// ** IO **
	in_t in;
	out_t out;

    // ** Operandos **
    ap_int<14 /* log2(I_MAX) */> ix;
	ap_int<8/* log2(O_BUS_b) */> i;
	ap_int<8/* log2(I_BUS_b) */> j;
	ap_int<IN_SIZE_b> data;

    // ** Operações **
	ap_int<MUL_BUS_b> mul;
	ap_int<ADD_BUS_b> add;

    float phi = (32*phi0)/(2*pi);
    float ps = 32*(*freq)*(*ti); /* phase step */

	do {
		in = strm_in.read();

		if(*dtype) {	/* real */
			ix = ((short int) phi) & 0x1F;
		
			for(k = 0; k <= IN_BUS_b; k += IN_SIZE_b) {
			
			
				MUL = cost[ix]*in.data;
			}

		} else { 		/* complex */

		}

		
		// TKEEP, define que bytes do BUS de entrada são válidos.
		k = 1;
		for (i = 0; i < IN_BUS_b; i += IN_SIZE_b) {
		#pragma HLS unroll
		#pragma HLS pipeline

			// Operandos
			data = in.data.range(i + IN_SIZE_b, i);

			// Produto Interno
			if (in.keep & k != 0)
				mult = a * b;
			else
				out.data.range(j + O_SIZE_b, j) = 0;

			// Controlo
			k = k << IN_T_BYTES;
		}

	} while();


	MUL[0] =
}
