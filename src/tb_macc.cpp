#include <hls_stream.h>

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define I_BUS_b  64
#define O_BUS_b 64

// ** Input Type **
#define I_T_b 16
#define I_T_B 2                                       // IN_T_b/8
#define O_SCALE 5

// ** Vectors **
#define MAX_IN 4                                      // I_BUS_b/I_T_b

// ** Operations **
#define MULT_SIZE 32                                  // 2*I_T_b
#define ACC_SIZE 64                                   // MULT_SIZE + ?

// ** IO **
typedef struct ap_axis<I_BUS_b, 0, 0, 0> in_t;
typedef struct ap_axis<O_BUS_b, 0, 0, 0> out_t;

// Top-level function
void macc(hls::stream<in_t> &strm_in_A, hls::stream<in_t> &strm_in_B, hls::stream<out_t> &strm_out);

// --------------------------- Test Bench  ----------------------------------

#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define VEC_SIZE_M 16384
#define VEC_SIZE 16368

#define MUTED 0

int main()
{
	/* TESTS
	ap_int<32> a = 32;
	ap_fixed<32,32-5> b;
	b.range(31,0) = a.range(31,0);
	printf("%f = %s\n", a.to_double(), a.to_string().c_str());
	printf("%f = %s\n", b.to_double(), b.to_string().c_str());
	printf("%f = %s\n", a.to_double(), ap_fixed<32,32-5>(a.range(31,0)).to_string().c_str());
	exit(-1);
	 */

	hls::stream<in_t> strm_in_A, strm_in_B;
	hls::stream<out_t> strm_out;
	in_t A, B;
	out_t O;

	ap_fixed<ACC_SIZE, ACC_SIZE - O_SCALE> tmp;
	double out_hw = 0;
	double out_sw = 0;
	int err = 0;

	FILE *II = fopen("../../../../../../data/sample/II.bin", "rb");
	FILE *QQ = fopen("../../../../../../data/sample/QQ.bin", "rb");
	FILE *CP = fopen("../../../../../../data/sample/code/PROMPT.bin", "rb");
	FILE *CE = fopen("../../../../../../data/sample/code/EARLY.bin", "rb");
	FILE *CL = fopen("../../../../../../data/sample/code/LATE.bin", "rb");

	FILE *u = II;
	FILE *v = CE;

	if(u == NULL || v == NULL)
		return -1;

	/* Write data to IP input stream  */

	short buffer_u[MAX_IN];
	short buffer_v[MAX_IN];

	if(!MUTED) printf("U\n");
	for(int k = 0; k < VEC_SIZE; k += MAX_IN) {
		fread(buffer_u,sizeof(short),MAX_IN,u);
		fread(buffer_v,sizeof(short),MAX_IN,v);
		A.strb = 1;
		B.strb = 1;
		for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++) {
			((short int*) &(A.data))[i] = buffer_u[i];
			((short int*) &(B.data))[i] = buffer_v[i];
			for(int j = 0; j < I_T_B - !i; j++)
				A.strb |= A.strb << 1;

			B.strb = A.strb;

			if(!MUTED) printf("strb = %s\n", A.strb.to_string().c_str());
		}

		if(k >= VEC_SIZE - MAX_IN)
			A.last = 1;
		else
			A.last = 0;

		B.last = A.last;

		for(int i = 0; (i < MAX_IN) && (k + i < VEC_SIZE); i++) {
			if(!MUTED) printf("%d ", ((short int*) &(A.data))[i]);
			if(!MUTED) printf("\t\t");
			if(!MUTED) printf("%d ", ((short int*) &(B.data))[i]);
			if(!MUTED) printf("\n");
		}

		A.keep = 0xFF;
		B.keep = 0xFF;

		strm_in_A.write(A);
		strm_in_B.write(B);
	}

	/* HW */

	macc(strm_in_A, strm_in_B, strm_out);

	O = strm_out.read();

	tmp.range(ACC_SIZE - 1, 0) = O.data.range(O_BUS_b - 1, 0);
	out_hw = tmp.to_double();

	if(!MUTED) printf("OUT_HW_64INT: %s\n", O.data.to_string().c_str());

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

	out_sw *= pow(2, -O_SCALE);

	/* Output */
	cout << "OUTPUT HW:" << endl;
	cout << out_hw << endl;
	cout << "OUTPUT SW:" << endl;
	cout << out_sw << endl;

	/* Check */
	err += (int) (out_hw - out_sw);

	if(!MUTED) printf("Error = %f\n", out_hw - out_sw);

	/* Close */
	fclose(II);
	fclose(QQ);
	fclose(CP);
	fclose(CE);
	fclose(CL);

	return err;
}
