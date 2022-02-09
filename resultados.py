from re import M
from numpy import array
from matplotlib.pyplot import scatter, plot, subplot, show, xlabel, ylabel, figure, title, legend, tight_layout

'''
# Latency
    pragma HLS loop_tripcount min=1024 max=16384 // directive only affects reports, not synthesis
                              ^ elements   ^ elements, adjusted in each src file to meet this limits in elements 
    ^ allow for comparison between synthesis
    ^ only applies to desgins with "latency": [min, max]
'''
Board = {
    "cortex_a9": {
        "frequency": 0.667, "frequency-max": 1, "cores": 2, "buses": "AMBA3",
        "word": 32, "simd": "NEON", "architecture": "ARMv7-A"
    },
    "artix": {
        "slices": 4416, "luts": 17600, "ram": 240, "dsp": 80, "speed": "-1"
    }
}

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
            - pipeline doesn't allow for proper unroll, but increases processing speed
            
            - testbench [IF_GN3S]: 82.225 ns (pipeline, unroll 2 + 2) vs 164.125 ns (no pipeline, unroll 4)
            """
    },
    "macc_v2": {},
    "macc_v1": {}
}

Implementation = {
    '''
    performance in miliseconds
    (ps time corresponds to the performance of the accelerated code section in the processor, for comparison)
    code used        : Satellite GPS 03
    <= 16368 samples : IF_GN3S.bin (sample)
    >  16368 samples : GPS_and_GIOVE_A-NN-fs16_3676-if4_1304.bin
    '''

    "macc_v3": {
        "name": "Correlation",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.302292, 0.321332, 0.361388, 0.439566, 0.587668, 0.933803, 4.327151, 16.119132]),
        "ps_time": array([0.302292, 0.077763, 0.157102, 0.439566, 0.626815, 1.251806, 9.661825, 38.512058])
    },
    "mixcarr_v1": {
        "name": "Mix Carrier",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.440775, 0.451794, 0.475634, 0.521034, 0.613129, 0.797868, 3.378058, 12.226458]),
        "ps_time": array([0.042111, 0.060225, 0.094129, 0.163794, 0.303640, 0.578335, 4.749695, 18.940545])
    },
    "rescode_v1": {
        "name": "Resample Code",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.057483, 0.080511, 0.126794, 0.218852, 0.403083, 0.772274, 5.932920, 23.628791]),
        "ps_time": array([0.030372, 0.059560, 0.117797, 0.234332, 0.467298, 0.934726, 7.464569, 29.857243])
    },
    "tracking_v1": {
        "name": "Signal Tracking",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.000000, 0.000000, 0.000000, 0.000000, 0.848182, 1.532458, 0.000000,  0.000000]),
        "ps_time": array([0.000000, 0.000000, 0.000000, 0.661446, 1.322283, 2.681591, 0.000000,  0.000000])
    }
}

impls = Implementation.values()

i = 1
figure()
 
for impl in impls:
    x = impl["samples"]
    f = impl["pl_time"]
    g = impl["ps_time"]
    
    if(i > 3):
        i = 1
        figure()

    subplot(3, 1, i)
    tight_layout()
    
    plot(x, f, '-og')
    plot(x, g, '-ob')
    
    legend(['PL', 'PS'])
    title(impl["name"])
    xlabel("Samples")
    ylabel("Time [ms]")
    
    i += 1

show()