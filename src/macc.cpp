#include <hls_stream.h>

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define I_BUS_b  64
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

// ** Memory **
static ap_uint<I_BUS_b> localmem[MAX_MEM];

/* 	** Top-level function **
 *
 *	Produto Interno
 *	strm_in: 	vectors: a,b
 *	strm_out:	scalar: a.b
 *	len:		len of input vector (assuming a vector 128 bits wide, then should be 1/8 of the short int vector size)
 */
void macc(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=strm_in
#pragma HLS interface axis port=strm_out

    // ** IO **
    in_t tmp;
    out_t tmpo;

    // ** Operands **
    ap_int<I_T_b> a, b;

    // ** Operations **
    ap_int<MULT_SIZE> mult;
    ap_int<ACC_SIZE> acc = 0;

    // ** Auxiliar **
    ap_uint<12> last; 							// log2(MAX_MEM)=12

    // control
    static ap_uint<12> h, i;                    // log2(MAX_VEC)=12
    static ap_uint<7> j;                        // log2(IN_BUS_SIZE)+1=7
    static ap_uint<(I_BUS_b + 7)/8> k;      	// dimensão de TSTRB (AXI)

    /* Behaviour */

    last = strm_in.read().data.range(11, 0); 	// FIRST ELEMENT RECEIVED IS THE (LENGTH - 1) <!>

    for (i = 0;; i++) {
        tmp = strm_in.read();                   // 1st vector read 	(a)
        localmem[i] = tmp.data;

        if(i == last)
			break;
    }

	for (i = 0;; i++) {
		tmp = strm_in.read();            		// 2nd vector read	(b)

		/* TSTRB : sets which BUS bytes are valid; each bit indicates one valid byte.
		 * I_T_B(=2): k has one HIGH bit corresponding to the less significant byte position associated to the iteration
		 * => AND OPERATION == 0 only when the integer is set as invalid in TSTRB.
		 */

		k = 1;
		for (j = 0; j < I_BUS_b; j += I_T_b) {
		#pragma HLS unroll // loop iterations are independent (except for the accumulation), which allows loop unrolling
		#pragma HLS pipeline

			// Operands
			a = localmem[i].range(j + 15, j);
			b = tmp.data.range(j + 15, j);

			// Dot product
			if ((tmp.strb & k) != 0)
				mult = a * b;
			else
				mult = 0;

			acc += mult;

			// Multiplier control
			k = k << I_T_B;
		}

		if(i == last)
			break;
	}

	// Results
	tmpo.data = acc;
	tmpo.data = tmpo.data >> SCALE;
	tmpo.last = 1;
	strm_out.write(tmpo);
}
