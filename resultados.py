from re import M
from numpy import array
from matplotlib.pyplot import scatter, plot, subplot, show, xlabel, ylabel, figure, title, legend, tight_layout, gcf

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
'''
    # Latency
        pragma HLS loop_tripcount min=1024 max=16384 // directive only affects reports, not synthesis
                                ^ elements   ^ elements, adjusted in each src file to meet this limits in elements 
        ^ allow for comparison between synthesis
        ^ only applies to desgins with "latency": [min, max]
'''
    "tracking_v1": {
        "notes":
            """
            - the DLL/FLL are running as SW in the CPU
            """
    },
    "rescode_v1": {

    },
    "mixcarr_v1": {
        "luts": 1193, "registers": 1386, "ram_percent": 3.94, "dsp": 0,
        "target_clock": 10, "hls_estimation": 8.724, "vivado_wns": 1.073, "latency": [4608, 73728],
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

Timing = {
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
        "pl_time": array([0.273911, 0.289095, 0.319526, 0.376742, 0.495902, 0.798963, 3.306446, 12.227212]),
        "ps_time": array([0.018498, 0.035646, 0.069634, 0.138458, 0.276594, 0.577262, 4.725815, 18.941006])
    },
    "rescode_v1": {
        "name": "Resample Code",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 131072  , 524288   ]),
        "pl_time": array([0.057483, 0.080511, 0.126794, 0.218852, 0.403083, 0.772274, 5.932920, 23.628791]),
        "ps_time": array([0.030372, 0.059560, 0.117797, 0.234332, 0.467298, 0.934726, 7.464569, 29.857243])
    },
    "tracking_v1": {
        "name": "Signal Tracking",
        "samples": array([511     , 1023    , 2046    , 4092    , 8184    , 16368   , 32768   , 65536    , 131072  , 262144   , 524288   ]),
        "pl_time": array([0.308615, 0.346951, 0.421963, 0.572766, 0.875714, 1.546603, 2.605286, 4.796529 , 9.216680, 18.064182, 35.830738]),
        "ps_time": array([0.084754, 0.166969, 0.330800, 0.660080, 1.324671, 2.668243, 5.414055, 11.039182,21.935212, 43.614591, 87.473988])
    }
}

impls = Timing.values()

i = 1
figure()

subplot_in_a_row = 4

for impl in impls:
    x = impl["samples"]
    f = impl["pl_time"]
    g = impl["ps_time"]
    
    if(i > subplot_in_a_row):
        i = 1
        figure()

    subplot(subplot_in_a_row/2, subplot_in_a_row/2, i)
    tight_layout()
    
    plot(x, g/f, '-og')
    #plot(x, g, '-ob')
    
    legend(['PL', 'PS'])
    title(impl["name"])
    xlabel("Samples")
    ylabel("Time [ms]")
    
    i += 1

gcf().suptitle("Speedup")

show()