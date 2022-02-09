#include <hls_stream.h>

#include <ap_int.h>
#include <ap_axi_sdata.h>

/* BUS */

#define I_BUS_b 32
#define O_BUS_b 64
#define LOG_I_BUS_b 5
#define LOG_O_BUS_b 6

/* ELEMENTS */

#define IO_T_b 16 // short int
#define IO_T_B 2
#define LOG_IO_T_b 4

#define MAX_SUPPORTED_EL 1024
#define LOG_MAX_SUPPORTED_EL 14

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

ap_int<IO_T_b> localmem[MAX_SUPPORTED_EL];

void rescode(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=strm_in
#pragma HLS interface axis port=strm_out

	// IO

	in_t in;
	out_t out;

	// Operands

	ap_fixed<64,16> offset;

	// Auxiliar

	ap_int<16> i;

	ap_int<15> m;

	ap_int<16> csize, np2;
	ap_int<24> lst;

	ap_uint<LOG_O_BUS_b+1> write = 0;
	ap_uint<1> stop_internal = 0;

	/*
	 *  ARGUMENTS
	*/

	/*** READ ***/
	in = strm_in.read();

	/* 1st : LAST OUTPUT INDEX = OUTPUT LENGTH - 1*/
	ap_int<24> last = in.data;

	/*** READ ***/
	in = strm_in.read();

	/* 2nd : CODE PHASE !! OF THE FIRST EARLY CODE !! = offset_0 + T_s * (- number_of_early_codes) * code_displacement */
	ap_fixed<64,16> offset_0; // Q16.48
	offset_0.range(31,0) = in.data;

	/*** READ ***/
	in = strm_in.read();

	offset_0.range(63,32) = in.data;

	/*** READ ***/
	in = strm_in.read();

	/* 3rd : SAMPLING PERIOD __IN CHIPS (CODE BITS)__ */
	ap_fixed<64,16> T_s = 0; // Q16.48
	T_s.range(31,0) = in.data;

	/*** READ ***/
	in = strm_in.read();

	T_s.range(47,32) = in.data.range(15,0);

	/* 4th : DISTANCE BETWEEN CODES IN SAMPLES (EARLY, PROMPT and LATE CODES) */
	ap_int<8> code_displacement = in.data.range(23,16); // in samples

	/* 5th : NUMBER OF CODE COPIES (e.g. EARLY, PROMPT and LATE) */
	ap_int<8> m_codes = in.data.range(31,24);

	/*** READ ***/
	in = strm_in.read();

	/* 6th : CSIZE */
	ap_int<16> csize_0 = in.data.range(15,0);

	csize = 0;
	m = 0;

	// Save the input code in local memory
	do {
		in = strm_in.read();
		for(i = 0; i < I_BUS_b; i += IO_T_b) {
			#pragma HLS unroll
			localmem[csize.range(LOG_MAX_SUPPORTED_EL - 1, 0)] = ap_int<IO_T_b>(in.data.range(i + IO_T_b - 1, i));

			// TSTRB identifies bytes which have data, there's a bit for each input byte
			// csize += in.strb.get_bit(i.range(15, LOG_IO_T_b - (IO_T_B - 1)));
			if(++csize >= csize_0) break;
		}
	} while(!in.last);

	offset = offset_0 + T_s * m * code_displacement;
	lst = last;

	// f(offset) : R --> [0, csize]
	offset = offset - ap_int<16>(offset/csize)*csize;
	if(offset < 0) offset += csize;
	// -- this solution adds 800 LUTS --

	// f(offset) : [-cszie, 2*csize]
	//if(offset < 0) offset += csize;
	//if(offset > csize) offset -= csize;

	// Output resampled code(s)
	do {
		for(write = 0; write < O_BUS_b; write += IO_T_b) {
			#pragma HLS unroll

			out.data.range((int) write + IO_T_b - 1, write) = localmem[offset.range(48 + LOG_MAX_SUPPORTED_EL - 1, 48)];

			offset += T_s;
			if(offset >= csize) offset -= csize;
		}

		out.strb = 0xFF;
		out.keep = 0xFF;

		if(lst-- == 0) {
			out.last = 1;

			m++;
			offset = offset_0 + T_s * m * code_displacement;
			if(offset < 0) offset += csize;
			else if(offset > csize) offset -= csize;

			lst = last;
		}
		else
			out.last = 0;

		strm_out.write(out);

	} while(m != m_codes);
}

