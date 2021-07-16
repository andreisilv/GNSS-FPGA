#include <ap_int.h>

#include <hls_stream.h>
#include <ap_axi_sdata.h>

// ** LOGICAL SIZES **

// IO Type
#define I_T_b				8
#define O_T_b				16		// > ADD_BUS_b

#define LOG_I_T_b 3

#define x2_I_T_b 16
#define x4_O_T_b 32

#define TRIG_ENTRIES		7
#define TRIG_ENTRIES_b		8
#define D090 8 //1*(TRIG_ENTRIES + 1)
#define D180 16 //2*(TRIG_ENTRIES + 1)
#define D270 24 //3*(TRIG_ENTRIES + 1)
#define D360 32 //4*(TRIG_ENTRIES + 1)

#define LOG_D360 5

// ** Other constants **
#define TRIG_AMPLITUDE 32
#define CMASK 0x1F
#define COMPLEX 1

// ** BUS SIZES **
#define I_BUS_b				32
#define O_BUS_b				128
#define MUL_BUS_b			10 		// most GNSS receivers A/DC quantize 4 bits (signed); trig vector (unsigned) uses 5 bits, 6 bits (signed)
#define ADD_BUS_b			16		// MUL_BUS_b + 1 = 11 ----same as output----> 16

#define LOG_I_BUS_b 5
#define LOG_O_BUS_b 8

#define TKEEP_MASK 0xFFFF
#define TSTRB_MASK 0xFFFF

// ** IO **
#define LENGTH_REGISTER_b 20

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

// ** LOCAL MEM **

const ap_int<TRIG_ENTRIES_b> trig[TRIG_ENTRIES] = {32, 31, 30, 27, 23, 18, 12, 6, 0, -6, -12, -18, -23, -27, -30, -31, -32, -31, -30, -27, -23, -18, -12,  -6, 0, 6, 12, 18, 23, 27, 30, 31};
//const ap_int<TRIG_ENTRIES_b> trig[TRIG_ENTRIES] = {31, 30, 27, 23, 18, 12, 6}; //   56 bits
const float pi = 3.1415926535897932384626; 	                        			 // + 32 bits
															                     // --------
			 												                     //   88 bits
																
// PI ~ 3.141592 = 0b011.0010010000111111011 -> 22 bits -> 21 bits (unsigned)
// PI ~ 3.14159  = 0b011.00100100001111111   -> 20 bits -> 19 bits (unsigned)
// PI ~ 3.1415	 = 0b011.00100100001111      -> 17 bits -> 16 bits (unsigned)
// PI ~ 3.14     = 0b011.001001              ->  9 bits ->  8 bits (unsigned)

/*
 * cost = (short) floor(32 * cos(2*PI/32 * i) + 0.5) =
 * = [32.,  31.,  30.,  27.,  23.,  18.,  12.,   6.,
 *     0.,  -6., -12., -18., -23., -27., -30., -31.,
 *   -32., -31., -30., -27., -23., -18., -12.,  -6.,
 *     0.,   6.,  12.,  18.,  23.,  27.,  30.,  31.]
 *
 * cost = [31, 30, 27, 23, 18, 12, 6]
 * max = TRIG_AMPLITUDE;
 * zero = 0;
 *
 *	 COS(x)
 *     0- 90 : {max,  cost[0:7]}
 *    90-180 : {zero, cost[7:0]*(-1)}
 *   180-270 : {max,  cost[0:7]*(-1)}
 *   270-360 : {zero, cost[0:7]}
 *
 *   SIN(x) = COS(x - 90)
*/

int mod360(int index)
{
	if(index > D360)
		return index - D360;
	else if(index <  0)
		return index + D360;
	else
		return index;
}

/*
* BUG: number of input elements must be even
*/

/*
*
*  Multiply the complex/real data with the local carrier.
*
*/

void mixcarr(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out)
{
#pragma HLS interface axis 			port=strm_in
#pragma HLS interface axis 			port=strm_out
#pragma HLS interface ap_ctrl_none 	port=return

    // ** IO **
	in_t in;
	out_t out;

    // ** Operands **
	ap_fixed<I_T_b> data[2];
	ap_fixed<TRIG_ENTRIES_b> cos;
	ap_fixed<TRIG_ENTIRES_b> sin;
	ap_fixed<O_BUS_b> result; // register with outputs of the adders

    // ** Operations **
	ap_fixed<MUL_BUS_b> mul[4];
	// there are adders

    // control
	float phase, pstep;
	ap_int<1> dtype = COMPLEX;
    static ap_uint<LOG_I_BUS_b> i;
    static ap_uint<LOG_O_BUS_b> j;
    static ap_uint<(I_BUS_b + 7)/8> k;      	// dimensão de TSTRB (AXI)
    static ap_uint<LOG_D360> index;
	ap_int<LENGTH_REGISTER_b - 1> last, processed = 0;

    /* Behaviour */

	// + 3 Cycles Latency, but less 83 bits in the BUS
	in = strm_in.read().data;
	last = in.range(LENGTH_REGISTER_b - 1, 0); 	// FIRST ELEMENT RECEIVED IS THE (LENGTH - 1) <!>
	dtype = in.range(I_BUS_b, I_BUS_b);			// THE LEAST BIT OF THE FIRST ELEMENT SETS THE DATA TYPE <!>

	in = strm_in.read().data;
	phase = in.read(); 							// SECOND ELEMENT RECEIVED IS THE PHASE <!>

	in = strm_in.read().data;
	pstep = in.read();							// THIRD ELEMENT RECEIVED IS THE PHASE_STEP <!>

	do {
		in = strem_in.read().data; // read 4 elements at once (32b)

		k = 1;
		for (i = 0, j = 0; i < I_BUS_b; i += x2_I_T_b, j += x4_O_T_b, phase += pstep) {
		#pragma HLS unroll
		#pragma HLS pipeline
			// loop over the 4 elements (each loop processes 2 elements)
            
			index = ((int) phase) & CMASK;

			// real case has double the throughput, skip to avoid bottleneck
			if(!dtype && (i & x4_O_T_b)) {

				// Operands
				data[0] = in.range(i + I_T_b - 1, i);
				data[1] = in.range(i + 2*I_T_b - 1, i + I_T_b);

				// Equal to zero if data is invalid
				if ((tmp.strb & k) != 0) {
					cos = trig[mod360(*index)];
					sin = trig[mod360(index - D090)];
				} else {
					cos = 0;
					sin = 0;
				}			

				// Operations
				mul[0] = cos*data[0];
				mul[1] = sin*data[0];
				mul[2] = cos*data[1];
				mul[3] = sin*data[1];

				if(dtype == COMPLEX) {
					result.range(j + O_T_b - 1, j) =  mul[0] - mul[3]; /* In quadrature component */
					result.range(j + 2*O_T_b - 1, O_T_b + j) = mul[1] - mul[2]; /* In phase component*/
				} else {
					result.range(j + O_T_b - 1, j) =  mul[0]; /* In quadrature component */
					result.range(j + 2*O_T_b - 1, O_T_b + j) = mul[1]; /* In phase component*/
					result.range(j + 3*O_T_b - 1, 2*O_T_b + j) = mul[2]; /* In phase component*/
					result.range(j + 4*O_T_b - 1, 3*O_T_b + j) = mul[3]; /* In quadrature component */
				}
			}

			// Validity control
			k = k << 2*I_T_B;
		}

		// the output are 8 elements (128b)
		out.data = result;
		out.keep = TKEEP_MASK;
		out.strb = k - 1;
		if(processed + 1 > last)
			out.last = 1;
		strm_out.write(out);

	} while(processed++ <= last);
}
