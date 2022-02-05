from numpy import array
from matplotlib.pyplot import scatter, plot, subplot, show, xlabel, ylabel, figure, title, legend, tight_layout

Board = {
    "cortex_a9": {
        "frequency": 0.667, "frequency-max": 1, "cores": 2, "buses": "AMBA3",
        "word": 32, "simd": "NEON", "architecture": "ARMv7-A"
    },
    "artix": {
        "slices": 4416, "luts": 17600, "ram": 240, "dsp": 80, "speed": "-1"
    }
}

'''
# NOTES
pragma HLS loop_tripcount min=1024 max=16384 // directive only affects reports, not synthesis
                          ^ elements   ^ elements, adjusted in each src file to meet this limits in elements 
^ allow for comparison between synthesis
^ only applies to desgins with "latency": [min, max]
'''

Design = {
    "tracking_v1": {
        "notes":
            """
            - the DLL/FLL are running as SW in the CPU
            """
    },
    "mixcarr_v1": {
    "luts": 1193, "registers": 1386, "ram_percent": 3.94, "dsp": 0,
    "target_clock": 10, "hls_estimation": 8.724, "vivado_wns": 1.073, "latency": [4615, 73735],
    "io": {"in_bus": 32, "out_bus": 128},
    "hls_directives": {
        "loop unroll": False,
        "pipeline": True, "initiation interval": "2",
        "loop flatten": False,
        "inline": True,
    },
    "notes":
        """
        - THIS BLOCK IS PART OF THE TRACKING ARCHITECTURE (should test individually too?)
        - only optimization "inline region": cycles: 17
                                                hls_estimation: 8.724 ns
                                                testbench [IF_GN3S]: 1.188095 ms
        - adding optimization pipeline II=2: cycles: 16
                                                hls_estimation: 8.658 ns
                                                testbench [IF_GN3S]: 0.532735 ms
        - still unable to do proper unrolling                                             
        """
    },
    "macc_v3": {
        "luts": 507, "registers": 739, "ram_percent": 2.10, "dsp": 8,
        "target_clock": 10, "hls_estimation": 7.900, "vivado_wns": 0.488, "latency": [515, 8195],
        "io": {"in_bus": 64, "out_bus": 64},
        "hls_directives": {
            "loop unroll": True, "unroll factor": 4,
            "pipeline": True, "initiation interval": 2,
            "loop flatten": False
        },
        "notes":
            """
            - THIS BLOCK IS PART OF THE TRACKING ARCHITECTURE (should test individually too?)
            - pipeline doesn't allow for proper unroll, but increases processing speed
              testbench [IF_GN3S]: 82.225 ns (pipeline, unroll 2 + 2) vs 164.125 ns (no pipeline, unroll 4)
            """
    },
    "macc_v2": {
        "luts": 507, "registers": 739, "ram_percent": 2.10, "dsp": 8,
        "additional_blocks": {
            "DMA_AXI" : {"luts": 3678, "register": 4765, "ram_percent": 16.25, "dsp": 0},
            "Floating Point (7.1)": {"luts": 290, "registers": 289, "ram_percent": 0.82, "dsp": 0}
        },
        "target_clock": 10, "hls_estimation": 6.914, "vivado_wns": 0.488, "latency": "13",
        "io": {"in_bus": 128, "out_bus": 64},
        "hls_directives": {
            "loop unroll": True, "unroll factor": 8,
            "pipeline": True, "initiation interval": "?",
            "loop flatten": False
        }
    }
}

Implementation = {
    '''
    performance in miliseconds
    (ps time corresponds to the performance of the accelerated code section in the processor, for comparison)
    code used        : Satellite GPS 03
    <= 16368 samples : IF_GN3S.bin (sample)
    >  16368 samples : GPS_and_GIOVE_A-NN-fs16_3676-if4_1304.bin
    "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288  ]),
    "pl_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000]),
    "ps_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000])
    '''

    "tracking_v1": {
        "name": "Tracking (v1)",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288  ]),
        "pl_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000]),
        "ps_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000])
    },
    "mixcarr_v1": {
        "name": "Mix Carrier (v1)",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.440775, 0.451794, 0.475634, 0.521034, 0.613129, 0.797868, 3.378058, 12.226458]),
        "ps_time": array([0.042111, 0.060225, 0.094129, 0.163794, 0.303640, 0.578335, 4.749695, 18.940545])
    },
    "macc_v3": {
        "name": "Correlation (v3)",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288  ]),
        "pl_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000]),
        "ps_time": array([0.000000, 0.000000, 0.00000, 0.000000 , 0.000000, 0.000000, 0.000000, 0.000000])
    },
    "macc_v2": {
        "name": "Correlation (v2)",
        "samples": array([127     , 255     , 511     , 1023    , 2046    , 4092    , 8184    , 16368   ]),
        "pl_time": array([0.300126, 0.314160, 0.343391, 0.400929, 0.517458, 0.749031, 1.203052, 2.166640]),
        "ps_time": array([0.009828, 0.018825, 0.038471, 0.077458, 0.155917, 0.312169, 0.621526, 1.246951])
    }
}

figure()
impls = Implementation.values()
i = 1
for impl in impls:
    x = impl["samples"]
    f = impl["pl_time"]
    g = impl["ps_time"]
    
    subplot(len(impls)/2, len(impls)/2, i)
    tight_layout()
    
    plot(x, f, '-og')
    plot(x, g, '-ob')
    
    legend(['PL', 'PS'])
    title(impl["name"])
    xlabel("Samples")
    ylabel("Time [ms]")
    
    i += 1

show()