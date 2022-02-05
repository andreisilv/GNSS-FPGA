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

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

void rescode(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out, ap_fixed<64,16> *step, ap_fixed<64,16> *offset, ap_int<32> *len, ap_int<32> *codelen)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=strm_in
#pragma HLS interface axis port=strm_out
#pragma HLS interface s_axilite port=step bundle=args
#pragma HLS interface s_axilite port=offset bundle=args
#pragma HLS interface s_axilite port=len bundle=args
#pragma HLS interface s_axilite port=codelen bundle=args

	if(*offset < 0) *offset += codelen;
	else if(*offset > codelen) *offset -= codelen;

	// IO

	in_t in;
	out_t out;

	// aux

	ap_int<16> j, k, h;

	ap_fixed<64,16> off1, off2 = *offset;
	ap_int<32> ln = *len;

	std::cout << *step << std::endl;
	std::cout << off2 << std::endl;
	std::cout << ln << std::endl;

	// Discard previous code chips
	for(off1 = 0; off1.range(63,48) < off2.range(63,48); off1 += (I_BUS_b/IO_T_b))
		in = strm_in.read();
	in = strm_in.read();

	j = off1.range(63,48) - off2.range(63,48);
	j = j << LOG_IO_T_b;
	h = *codelen - off1.range(63,48);

	off1 = off2;

	// Output resampled code
	do {
		for(k = 0; k < O_BUS_b; k += IO_T_b) {
			#pragma HLS unroll

			std::cout << j << std::endl;
			std::cout << off2 << std::endl;
			std::cout << off1 << std::endl;
			std::cout << "-----" << std::endl;

			out.data.range(k + IO_T_b - 1, k) = in.data.range(j + IO_T_b - 1, j);

			off2 += *step;

			if(off2.range(63,48) > off1.range(63,48)) {
				j += IO_T_b;
				off1 = off2;

				if(j >= I_BUS_b) {
					j = 0;
					in = strm_in.read();
				}
			}
		}

		out.strb = 0xFF;
		out.keep = 0xFF;

		out.last = !ln;
		strm_out.write(out);
		ln -= (I_BUS_b/IO_T_b);

	} while(ln > 0);

	// Discard following code chips
	for(off1 = 0; off1 < h; off1 += (I_BUS_b/IO_T_b))
		in = strm_in.read();

	*offset += *step;

	if(*offset < 0) *offset += codelen;
	else if(*offset > codelen) *offset -= codelen;
}

