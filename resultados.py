from re import M
from numpy import array
from matplotlib.pyplot import scatter, plot, subplot, show, xlabel, ylabel, figure, title, legend, tight_layout, gcf, grid

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
        "luts": 3890, "registers": 5228, "ram_percent": 11.45, "dsp": 13,
        "target_clock": 10, "vivado_wns": 0.254,
        "dma overhead": {
            "luts": 4818, "registers": 6560, "ram_percent": 25.45, "dsp": 0
        },
        "notes":
            """
            - main stats: includes: broadcasters, macc, mixcarr, rescode, floating point units
            - the DLL/FLL are running as SW in the CPU
            """
    },
    "rescode_v1": {
        "luts": 1205, "registers": 1712, "ram_percent": 4.86, "dsp": 5,
        "target_clock": 10, "hls_estimation": 8.715, "vivado_wns": 2.204, "latency": [0, 0],
        "io": {"in_bus": 32, "out_bus": 64},
        "hls_directives": {
            "loop unroll": True, "unroll factor": 2,
            "pipeline": False,
            "loop flatten": False,
            "inline": False,
        },
        "dma overhead": {
            "luts": 2738, "registers": 3743, "ram_percent": 14.4, "dsp": 0
        },
        "notes":
            """
            """
    },
    "mixcarr_v1": {
        "luts": 1174, "registers": 1384, "ram_percent": 3.93, "dsp": 0,
        "target_clock": 10, "hls_estimation": 8.724, "vivado_wns": 1.401, "latency": [2311, 36871],
        "io": {"in_bus": 32, "out_bus": 128},
        "hls_directives": {
            "loop unroll": False,
            "pipeline": True, "initiation interval": "2",
            "loop flatten": False,
            "inline": True,
        },
        "dma overhead": {
            "luts": 5282, "registers": 7677, "ram_percent": 21.32, "dsp": 0
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
        "luts": 356, "registers": 367, "ram_percent": 1.04, "dsp": 4,
        "target_clock": 10, "hls_estimation": 8.665, "vivado_wns": 0.519, "latency": [256, 4096],
        "io": {"in_bus": 64, "out_bus": 64},
        "hls_directives": {
            "loop unroll": True, "unroll factor": 2+2,
            "pipeline": True, "initiation interval": 2,
            "loop flatten": False
        },
        "dma overhead": {
            "luts": 4306, "registers": 5888, "ram_percent": 16.73, "dsp": 0
        },
        "floating poit unit overhead": {
            "luts": 320, "registers": 508, "ram_percent": 1.44, "dsp": 0
        },
        "notes":
            """
            - pipeline doesn't allow for proper unroll, but increases processing speed (splits loop unroll 4 => 2 + 2)
            - testbench [IF_GN3S]: 82.225 ns (pipeline, unroll 2 + 2) vs 164.125 ns (no pipeline, unroll 4)
            """
    },
    "macc_v2": {},
    "macc_v1": {}
}

Timing = {
    "macc_v3": {
        "name": "Correlation",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([1023    , 2046    , 4092    , 8184    , 16368   ]),
        "y_pl": array([0.321332, 0.373800, 0.455292, 0.610040, 0.973354]),
        "y_ps": array([0.077763, 0.142766, 0.286480, 0.577711, 1.153388])
    },
    "macc_v2": {
        "name": "Correlation (v2)",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([16368   ]),
        "y_pl": array([2.140825]),
        "y_ps": array([1.153622])
    },
    "macc_v1": {
        "name": "Correlation (v1)",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([1023    , 2046    , 4092    , 8184    , 16368   ]),
        "y_pl": array([0.358849, 0.425677, 0.551778, 0.798049, 1.347049]),
        "y_ps": array([0.071775, 0.143212, 0.287360, 0.578788, 1.153591])
    },
    "mixcarr_v1": {
        "name": "Mix Carrier",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([1023    , 2046    , 4092    , 8184    , 16368   ]),
        "y_pl": array([0.289095, 0.319526, 0.376742, 0.495902, 0.798963]),
        "y_ps": array([0.035646, 0.069634, 0.138458, 0.276594, 0.577262])
    },
    "rescode_v1": {
       "name": "Resample Code",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([1023    , 2046    , 4092    , 8184    , 16368   ]),
        "y_pl": array([0.080511, 0.126794, 0.218852, 0.403083, 0.772274]),
        "y_ps": array([0.059560, 0.117797, 0.234332, 0.467298, 0.934726])
    },
    "tracking_v1": {
        "name": "Signal Tracking",
        "x_l": "Samples",
        "y_l": "Time [ms]",
        "x":    array([1023    , 2046    , 4092    , 8184    , 16368   ]),
        "y_pl": array([0.346951, 0.421963, 0.572766, 0.875714, 1.546603]),
        "y_ps": array([0.166969, 0.330800, 0.660080, 1.324671, 2.668243])
    }
}

'''
performance in miliseconds
(ps time corresponds to the performance of the accelerated code section in the processor, for comparison)
code used        : Satellite GPS 03
<= 16368 samples : IF_GN3S.bin (sample)
>  16368 samples : GPS_and_GIOVE_A-NN-fs16_3676-if4_1304.bin
'''

i = 1
figure()

layout_i = 2
layout_j = 2

layout = ["macc_v1", "macc_v2", "macc_v3"]
# layout = ["tracking_v1", "macc_v3", "rescode_v1", "mixcarr_v1"]

for key in layout:
    impl = Timing[key]
    x = impl["x"]
    f = impl["y_pl"]
    g = impl["y_ps"]

    subplot(layout_i, layout_j, i)
    tight_layout()
    
    plot(x, g/f, '-og')
    #plot(x, g, '-ob')
    
    legend(['PL', 'PS'])
    title(impl["name"])
    xlabel(impl["x_l"])
    ylabel("Speedup")
    grid()
    
    i += 1

gcf().suptitle("")

# show()