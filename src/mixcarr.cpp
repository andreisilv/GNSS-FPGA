#include <ap_int.h>

#include <hls_stream.h>
#include <ap_axi_sdata.h>

// ** LOGICAL SIZES **

// IO Type
#define I_T_b			8
#define O_T_b			16	// > ADD_BUS_b

#define O_T_MASK  0x3
#define LOG_I_T_b 3
#define LOG_O_T_b 4

// Local Carrier Table

#define COST_ENTRIES	8
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
#define O_TSTRB_b			((O_BUS_b + 7)/8) 	// TSTRB width (AXI) ( valid bytes control )
#define MUL_BUS_b			10 					// most GNSS receivers A/D quantize 4 bits (signed) // carrier local table 5 bits (unsigned), 6 bits (signed)
#define ADD_BUS_b			11					// MUL_BUS_b + 1 = 11

#define LOG_I_BUS_b 5
#define LOG_O_BUS_b 8

/* TSTRB=HIGH && TKEEP=HIGH : "The associated byte contains valid information that must be transmitted between source and destination."
 * If TSTRB is LOW, then data is invalid, however if TKEEP is HIGH, the indicated byte should be kept: "The associated byte indicates the relative position of the data bytes in a stream, but does not contain any relevant data values."
 */
#define TKEEP_MASK 0xFFFF
#define TSTRB_MASK 0xFFFF

// ** IO **
#define LENGTH_REGISTER_b 20

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

// ** LOCAL MEM **

const ap_int<COST_ENTRIES_b> cost[COST_ENTRIES] = {32, 31, 30, 27, 23, 18, 12, 6}; //const ap_int<COST_ENTRIES_b> cost[32] = {32,  31,  30,  27,  23,  18,  12,   6, 0,  -6, -12, -18, -23, -27, -30, -31, -32, -31, -30, -27, -23, -18, -12,  -6, 0,   6,  12,  18,  23,  27,  30,  31};
const float pi = 3.1415926535897932384626; // PI ~ 3.141592 = 0b011.0010010000111111011 -> 22 bits -> 21 bits (unsigned) // PI ~ 3.14159  = 0b011.00100100001111111 -> 20 bits -> 19 bits (unsigned) // PI ~ 3.1415 = 0b011.00100100001111 -> 17 bits -> 16 bits (unsigned) // PI ~ 3.14 = 0b011.001001 ->  9 bits ->  8 bits (unsigned)

/*
 * cost = (short) floor(32 * cos(2*PI/32 * i) + 0.5) =
 *		= [32,  31,  30,  27,  23,  18,  12,   6,		// 0-7 // 00000-00111
 *			0,  -6, -12, -18, -23, -27, -30, -31,       // 8-15// 01000-01111
 *		  -32, -31, -30, -27, -23, -18, -12,  -6,       //16-23// 10000-10111
 *			0,   6,  12,  18,  23,  27,  30,  31]       //24-31// 11000-11111
 *
 * cost = [31, 30, 27, 23, 18, 12, 6]
 * max = AMPLITUDE;
 * zero = 0;
 *
 *	 COS(x)
 *     0- 90 : {max,  cost[0:6]}
 *    90-180 : {zero, cost[6:0]}*(-1)
 *   180-270 : {max,  cost[0:6]}*(-1)
 *   270-360 : {zero, cost[6:0]}
 *
 *   SIN(x) = COS(x - 90)
 *
*/

// ** CORRELATOR MEM **

#define MAX_VEC 16384

int cosseno(ap_uint<INDEX_b> index)
{
	if(index == 8 || index == 24)
		return 0;

	if(index.get_bit(3)) {
		if(index.get_bit(3) ^ index.get_bit(4))
			return 0 - cost[(0 - index).range(2,0)];
		else
			return cost[(0 - index).range(2,0)];
	} else {
		if(index.get_bit(3) ^ index.get_bit(4))
			return 0 - cost[index.range(2,0)];
		else
			return cost[index.range(2,0)];
	}
}

/*
*
*  Multiply complex/real data with a local carrier.
*
*	II = cos[index]*p[0] - sin[index]*p[1]
*   QQ = sin[index]*p[0] + cos[index]*p[1]
*
*/

void mixcarr(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out)
{
#pragma HLS interface axis 			port=strm_in
#pragma HLS interface axis 			port=strm_out
#pragma HLS interface ap_ctrl_none 	port=return

	/*
	// DEBUG LOCAL TABLE
	for(int i = -32; i < 64; i++)
		printf("cos[%d]=%d\n", i, cosseno(i));
	exit(-1);
	*/

    // ** IO **

	in_t in;
	out_t out;

    // ** Operands **

	ap_int<I_T_b> data[2];

    // ** Operations **

	ap_int<MUL_BUS_b> mul[4];
	ap_int<ADD_BUS_b> add[2];
	ap_int<MUL_BUS_b> cos[2], sin[2];

    // control

    ap_int<LENGTH_REGISTER_b - 1> last, processed = 0;
    ap_int<11/*log(MAX_VEC)*/> correlated = 0;
	ap_int<1> dtype;
	ap_fixed<64,20> phase;
	ap_fixed<64,20> pstep;
	ap_int<INDEX_b> index;

	static ap_uint<LOG_I_BUS_b+1> i;
    static ap_uint<LOG_O_BUS_b+1> j;
    static ap_uint<O_TSTRB_b/2 + 1> strb; // +1 to have space for the carry  // SHIFT REGISTER	[CARRY=INPUT_TSTRB(k)]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]
    static ap_uint<4 /*log(O_TSTRB_b/2 + 1)=log(9)~4*/> k;
    static ap_uint<8 /*sufficient*/> w;

	/*
	 * (!) ARGUMENTS READ ONE AT A TIME (for a 32 bits input)
	 */

	// 1st: last = length - 1
	in = strm_in.read();
	last = in.data;

	// 2nd: data type
	in = strm_in.read();
	dtype = in.data.get_bit(0);

	// 3rd: carrier phase (of width 64, read in two steps; in order to increase precision)
	in = strm_in.read();
	phase.range(31,0) = in.data;
	in = strm_in.read();
	phase.range(63,32) = in.data;

	// 4th: phase step (of width 64, read in two steps; in order to increase precision)
	in = strm_in.read();
	pstep.range(31,0) = in.data;
	in = strm_in.read();
	pstep.range(63,32) = in.data;

	do {// while there are real/complex elements to process
		in = strm_in.read();

		/* LOOP
		 *
		 * Since multiple data is received simultaneously the loop can be unrolled.
		 * At least two elements of data are read if data is complex. To simplify
		 * loop unrolling, all iterations can utilize two input elements
		 * and differentiate between complex and real in how many outputs are
		 * calculated.
		 */

		if(correlated++ == 0) {
			out.data.range(31, 0) = last;
			out.data.range(95, 64) = last;
			out.keep = 0x0F0F;
			out.strb = 0x0F0F;
			out.last = 1;
			strm_out.write(out);
		}

		j = 0; k = 0;
		for (i = 0; i < I_BUS_b; i += I_T_b * 2) {
			#pragma HLS unroll
			#pragma HLS inline region
			//#pragma HLS pipeline

			if(j >= O_BUS_b/2)
				j = 0;

			// ** Operands **

			index = phase;
			cos[0] = cosseno(index);
			sin[0] = cosseno(index - D90);

			if(dtype == REAL) {
				index = phase + pstep;
				cos[1] = cosseno(index);
				sin[1] = cosseno(index - D90);
			} else {
				cos[1] = cos[0];
				sin[1] = sin[0];
			}

			data[0] = in.data.range(i + I_T_b - 1, i);
			data[1] = in.data.range(i + 2*I_T_b - 1, i + I_T_b);

			// ** Operations **

			mul[0] = cos[0]*data[0];
			mul[1] = sin[0]*data[0];
			mul[2] = cos[1]*data[1];
			mul[3] = sin[1]*data[1];

			/* OUTPUT:
			   [                    128b                   ]
			   [      IN PHASE       ][   IN QUADRATURE    ]
			*/
			for(w = 0; w < O_T_b/8; w ++) {
				#define HLS unroll
				strb.set_bit(8, in.strb.get_bit(k)); 											// SHIFT REGISTER	[CARRY=TSTRB(k)]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]
				strb = strb >> 1;
			}
			//printf("strb = %s\n", strb.to_string().c_str());

			if(dtype == COMPLEX) {
				add[0] = mul[0] - mul[3];
				add[1] = mul[1] - mul[2];

				out.data.range(j + O_T_b - 1, j) = add[0]; 										// II[i]: 	In quadrature component
				out.data.range(O_BUS_b/2 + j + O_T_b - 1, O_BUS_b/2 + j) = add[1];				// QQ[i]: 	In phase component
			} else {
				for(w = 0; w < O_T_b/8; w ++) {
					#define HLS unroll
					strb.set_bit(8, in.strb.get_bit(k+I_T_b/8));								// SHIFT REGISTER	[CARRY=TSTRB(k)]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]->[ ]
					strb = strb >> 1;
				}
				//printf("strb = %s\n", strb.to_string().c_str());

				out.data.range(j + O_T_b - 1, j) =  mul[0]; 									// II[i]: 	In phase component
				out.data.range(O_BUS_b/2 + j + O_T_b - 1, O_BUS_b/2 + j) = mul[1];	 			// QQ[i]: 	In quadrature component
				out.data.range(j + 2*O_T_b - 1, O_T_b + j) = mul[2]; 							// II[i+1]: In phase component
				out.data.range(O_BUS_b/2 + j + 2*O_T_b - 1, O_BUS_b/2 + O_T_b + j) = mul[3]; 	// QQ[i+1]: In quadrature component
			}

			// Step
			j += O_T_b << (dtype == REAL);
			k += I_T_b/4 /* 2*(I_T_b/8) */;
			phase += pstep << (dtype == REAL);
		}

		// Output
		if(j >= O_BUS_b/2) {
			out.keep = TKEEP_MASK;
			//out.strb = TSTRB_MASK;
			out.strb.range(O_TSTRB_b - 1, O_TSTRB_b/2) = strb; // II and QQ have the same TSTRB
			out.strb.range(O_TSTRB_b/2 - 1, 0) = strb;
			strb = 0;
			//printf("WRITTEN out.strb = %s\n", out.strb.to_string().c_str());
			out.last = 0;
			if((processed + 1 > last) || (correlated == 0))
				out.last = 1;
			//printf("%s > %s = %s\n", (processed + 1).to_string().c_str(), last.to_string().c_str(), out.last.to_string().c_str());
			strm_out.write(out);
		}

	} while(processed++ < last);
}
