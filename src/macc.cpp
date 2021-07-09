#include <hls_stream.h>

#include <ap_int.h>
#include <ap_axi_sdata.h>

// ** Buses **
#define IN_BUS_SIZE  128
#define OUT_BUS_SIZE 64

// ** Input Type **
#define IN_T_BITS 16
#define IN_T_BYTES 2                                        // IN_T_BITS/8

// ** IO **
typedef struct ap_axis<IN_BUS_SIZE, 0, 0, 0> in_t;
typedef struct ap_axis<OUT_BUS_SIZE, 0, 0, 0> out_t;

// ** Vetores **
#define MAX_IN 8                                            // IN_BUS_SIZE/IN_T_BITS
#define MAX_VEC 16384
#define MAX_MEM 2048                                        // MAX_VEC*(IN_T_BITS/IN_BUS_SIZE)

// ** Operaçõees **
#define MULT_SIZE 32                                        // 2*IN_T_BITS
#define ACC_SIZE 43                                         // MULT_SIZE + 11 // log2(MAX_MEM)

// ** Memória **
static ap_uint<IN_BUS_SIZE> localmem[MAX_MEM];

/* 	** Top-level function **
 *
 *	Produto Interno
 *	strm_in: 	vetores: a,b
 *	strm_out:	um elemento escalar: a.b
 */
void macc(hls::stream<in_t> &strm_in, hls::stream<out_t> &strm_out)
{
#pragma HLS interface ap_ctrl_none port=return
#pragma HLS interface axis port=strm_in
#pragma HLS interface axis port=strm_out

    // ** IO **
    in_t tmp;
    out_t tmpo;

    // ** Operandos **
    ap_int<IN_T_BITS> a, b;

    // ** Operações **
    ap_int<MULT_SIZE> mult;
    ap_int<ACC_SIZE> acc;

    // ** Auxiliar **

    // nº de pacotes recebidos                  -- BUS 128 BITS
    static ap_uint<14> pkts;                    // log2(MAX_VEC)=14

    // controlo
    static ap_uint<14> i;                       // log2(MAX_VEC)=14
    static ap_uint<8> j;                        // log2(IN_BUS_SIZE)=8
    static ap_uint<(IN_BUS_SIZE + 7)/8> k;      // dimensão de TKEEP (AXI)

    for (i = 0; i < MAX_MEM; i++) {
        tmp = strm_in.read();                   // Leitura do primeiro vetor 	(a)
        localmem[i] = tmp.data;
        if (tmp.last == 1) break;
    }
    pkts = ++i;

    do {
        acc = 0;

        for (i = 0; i < pkts; i++) {
        #pragma HLS loop_flatten

            tmp = strm_in.read();               // Leitura do segundo vetor		(b) (em pipeline com o produto interno)

            /* TKEEP : O sinal define que bytes do BUS de entrada são válidos; cada bit indica a validade de um byte.
             * IN_T_BYTES(=2): O valor k fica com um único bit a '1' na posição do byte menos significativo associado à iteração
             * logo AND é =0 apenas quando o inteiro recebido for indicado como inválido em TKEEP.
             */
            k = 1;
            for (j = 0; j < IN_BUS_SIZE; j += IN_T_BITS) {
            #pragma HLS unroll
            #pragma HLS pipeline

                // Operandos
                a = localmem[i].range(j + 15, j); // um único elemento é lido durante o loop, pelo que o unroll não utiliza memória adicional para carregar os elementos
                b = tmp.data.range(j + 15, j);

                // Produto Interno
                if (tmp.keep & k != 0)
                    mult = a * b;
                else
                    mult = 0;
                acc += mult;

                // Controlo do Multiplicador
                k = k << IN_T_BYTES;
            }
        }

        // Resultado
        tmpo.last = tmp.last;
        tmpo.keep = 0xff; // indica que todos os bytes estão válidos
        tmpo.strb = tmpo.keep; // strb indica o mesmo que o tkeep
        tmpo.data = acc;
        strm_out.write(tmpo);

    } while (tmp.last != 1);
}
