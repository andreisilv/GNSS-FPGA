#include <hls_stream.h>

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define I_BUS_b 64
#define O_BUS_b 64

// ** Input Type **
#define I_T_b 16
#define LOG_I_T_b 4
#define O_SCALE 5

// ** Vectors **
#define MAX_IN (I_BUS_b/I_T_b)
#define LOG_MAX_IN 2

// ** Operations **
#define MULT_SIZE (2*I_T_b)
#define ACC_SIZE 64

// ** IO **
typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

/* 	** Top-level function **
 *
 *	Produto Interno
 *	strm_in: 	vectors: a,b
 *	strm_out:	scalar: a.b
 */

void macc(hls::stream<in_t> &strm_in_A, hls::stream<in_t> &strm_in_B, hls::stream<out_t> &strm_out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=strm_in_A
#pragma HLS interface axis port=strm_in_B
#pragma HLS interface axis port=strm_out

    // ** IO **
    in_t A, B;
    out_t O;

    // ** Operands **
    ap_int<I_T_b> b[MAX_IN], d[MAX_IN];
    static ap_int<1> z[MAX_IN];

    // ** Operations **
    ap_int<ACC_SIZE> acc[MAX_IN];

    // control
    static ap_uint<8> i, j;                 			// log2(IN_BUS_SIZE)+1=7+1
    static ap_uint<8> middle, lap, mod;

    /* Behaviour */

	O.data = 0; // reset accumulator

	do {
		#pragma HLS loop_tripcount min=256 max=4096 // directive only affects reports, not synthesis

		//#pragma HLS loop_flatten (no additional effect, all the inner loops are unrolled correctly)
		#pragma HLS pipeline II=1 // (side effect: splits loop unroll 4 => 2 + 2)

		A = strm_in_A.read();            				// 1st vector read	(b)
		B = strm_in_B.read();            				// 2nd vector read	(d)

		for (i = 0; i < I_BUS_b; i += I_T_b) {
			#pragma HLS unroll skip_exit_check		 	// loop iterations are independent allowing loop unrolling

			// Operands
			j = i.range(7, LOG_I_T_b);					// 'j': represents the current element being processed
			b[j] = A.data.range(i + I_T_b - 1, i);
			d[j] = B.data.range(i + I_T_b - 1, i);
			// TSTRB is an AXI signal - it has a bit for each input byte - each bit at HIGH indicates the corresponding byte contains data
			z[j] = A.strb.get_bit(i.range(7, 3)) & B.strb.get_bit(i.range(7, 3));

			// Dot product
			acc[j] = (b[j] * d[j]) & z[j]; // 1 DSP
		}

		/* Because it's simple to implement modular operations of base 2 in hardware
		 * a ring can be used to do the summation, where each opposite element
		 * is summed. Then, the ring size is divided by two and the process repeated.
		*/
		for(i = 0; i < MAX_IN; i++) {
			#pragma HLS unroll skip_exit_check

			lap = i.range(7, LOG_MAX_IN - 1);
			middle = (MAX_IN/2) >> lap;
			mod = LOG_MAX_IN - 1 - lap;
			acc[i.range(mod, 0)] += acc[(i + middle).range(mod, 0)];
			/* HARDCODED
			acc[0] += acc[2];
			acc[1] += acc[3];
			acc[0] += acc[1];
			*/
		}

		O.data += acc[0];

	} while(!A.last /* | !B.last */);

	// Results
	O.keep = 0xFF;
	O.strb = 0xFF;
	O.last = 1;
	strm_out.write(O);
}
