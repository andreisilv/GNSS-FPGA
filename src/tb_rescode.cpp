#include <hls_stream.h>

#include <ap_int.h>
#include <ap_axi_sdata.h>

/* BUS */

#define I_BUS_b 32
#define O_BUS_b 64

/* ELEMENTS */

#define IO_TYPE_b 16 // short int
#define IO_TYPE_B 2

typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

// --------------------------- IP  ----------------------------------

void rescode(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out);

// --------------------------- Test Bench  ----------------------------------

#include <stdio.h>
#include <stdlib.h>

#define SHORT(X) ((short) ( !(X & 0x80) ?  X : X | 0xFF00 ))

#define CODE_SIZE 1023
#define RESAMPLED_SIZE 4092//16368

#define MUTE 0

int main()
{

	FILE *fcode = fopen("../../../../../../data/sample/code/code.bin", "rb");
	FILE *fcodE = fopen("../../../../../../data/sample/code/EARLY.bin", "rb");
	FILE *fcodP = fopen("../../../../../../data/sample/code/PROMPT.bin", "rb");
	FILE *fcodL = fopen("../../../../../../data/sample/code/LATE.bin", "rb");
	if(fcode == NULL || fcodE == NULL || fcodP == NULL || fcodL == NULL) {
		printf("FILE ERROR\n");
		exit(-1);
	}
	FILE *ftest[3] = {fcodE, fcodP, fcodL};

	short int buffer[O_BUS_b/IO_TYPE_b];
	short int res;
	int err;

	ap_uint<1> stop_in, stop_out;

	/* 1st */ ap_int<16> last = RESAMPLED_SIZE/(O_BUS_b/IO_TYPE_b) - 1;
	/* 2nd */ ap_fixed<64,16> offset_0 = ap_fixed<64,16>(0 - 8*1023000*6.109482e-08);
	/* 3rd */ ap_fixed<64,16> T_s = ap_fixed<64,16>(1023000*6.109482e-08);
	/* 4th */ ap_int<8> code_displacement = 8; // 16 samples per chip
	/* 5th */ ap_int<8> m_codes = 3; // early code, prompt code, late code
	/* 6th */ ap_int<32> csize = CODE_SIZE; // code length

    hls::stream<in_t> strm_in;
   	hls::stream<out_t> strm_out;

	in_t arg, in;
	out_t out;

	/* ARGUMENTS */

	arg.data = last;
	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	arg.data = offset_0.range(31,0);
	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	arg.data = offset_0.range(63,32);
	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	arg.data = T_s.range(31,0);
	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	arg.data.range(15,0) = T_s.range(47,32);
	arg.data.range(23,16) = code_displacement;
	arg.data.range(31,24) = m_codes;

	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	arg.data = csize;
	arg.strb = 0xF;
	arg.keep = 0xF;
	arg.last = 1;
	strm_in.write(arg); 	/* WRITE */

	int h, i, j, bus_size;

	/* Write Data  */

	bus_size = I_BUS_b/IO_TYPE_b;

	for(i = 0; i < CODE_SIZE; i += bus_size) {

	   fread(buffer, sizeof(short int), bus_size, fcode);

	   for(int j = 0; j < bus_size; j++)
		   ((short int*) &(in.data))[j] = buffer[j];

	   in.keep = 0xF;
	   in.strb = i >= CODE_SIZE - bus_size ? 0x3 : 0xF;
	   in.last = i >= CODE_SIZE - bus_size ? 1 : 0;

	   strm_in.write(in);
	}

	/* HW */
	rescode(strm_in, strm_out);

	/* Test */

	bus_size = O_BUS_b/IO_TYPE_b;

	for(h = 0; h < 3; h++) {
		if(!MUTE) printf("~~~~~~~~~~~~~~~~~~~~~~~~ CODE %d ~~~~~~~~~~~~~~~~~~~~~~~~\n", h);

		for(i = 0; i < RESAMPLED_SIZE; i += bus_size) {
			out = strm_out.read();

			fread(buffer, sizeof(short int), bus_size, ftest[h]);
			for(int j = 0; j < bus_size; j++) {
				res = ((short int*) &(out.data))[j];

				if(res != buffer[j]) {
					err ++;
					if(!MUTE) printf("######################## ERROR ########################\n");
					if(!MUTE) printf("[%d] %d\t%d\n", i + j, res, buffer[j]);
				}
			}
		}
		if(!MUTE) printf("\n\n\n\n");
	}

	fclose(fcode);
	fclose(fcodE);
	fclose(fcodP);
	fclose(fcodL);

	if(!MUTE) printf("err = %d\n", err);
	return err > 10;
}
