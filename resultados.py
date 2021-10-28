from numpy import array
from matplotlib.pyplot import scatter, plot, show, xlabel, ylabel, figure, title

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
    "macc_v2": {
        "luts": 507, "registers": 739, "ram_percent": 2.10, "dsp": 8,
        "additional_blocks": {
            "DMA_AXI" : {"luts": 3678, "register": 4765, "ram_percent": 16.25, "dsp": 0},
            "Floating Point (7.1)": {"luts": 290, "registers": 289, "ram_percent": 0.82, "dsp": 0}
        },
        "target_clock": 10, "hls_estimation": 6.914, "vivado_wns": 0.488,
        "latency": 13, "axi": "stream",
        "io": {"in_bus": 128, "out_bus": 64},
        "hls_directives": {
            "loop unroll": True, "unroll factor": 8,  "pipeline": True, "loop flatten": True
        }
    },
    "mixcarr_v1": {
        # dsp values is zero? report bug?
        "luts": 1286, "registers": 1405, "ram_percent": 3.99, "dsp": 0,
        "additional_blocks": {
            "DMA_AXI" : {"luts": 4754, "register": 6885, "ram_percent": 9.63, "dsp": 0},
            "Broadcaster": {"luts": 4, "registers": 2, "ram_percent": 0.01, "dsp": 0}
        },
        "target_clock": 10, "hls_estimation": 8.632,
        "latency": 27, "axi": "stream",
        "io": {"in_bus": 32, "out_bus": 128}, "vivado_wns": 1.318,
        "hls_directives": {
            "loop unroll": True, "unroll factor": 2, "pipeline": False, "loop flatten": False
        }
    }
}

Testbench = {
    # performance in miliseconds
    # (ps time corresponds to the performance of the accelerated code section in the processor, for comparison)
    # code used        : Satellite GPS 03
    # <= 16368 samples : IF_GN3S.bin (sample)
    # 10000000 samples : GPS_and_GIOVE_A-NN-fs16_3676-if4_1304.bin

    "tracking correlator": {
        "macc_v2": {
            "name": "Correlation",
            "samples": array([127, 255, 511, 1023, 2046, 4092, 8184, 16368]),
            "pl_time": array([0.300126, 0.314160, 0.343391, 0.400929, 0.517458, 0.749031, 1.203052, 2.166640]),
            "ps_time": array([0.009828, 0.018825, 0.038471, 0.077458, 0.155917, 0.312169, 0.621526, 1.246951])
        },
        "mixcarr_v1": {
            "name": "Mix Carrier",
            "samples": array([127, 255, 511, 1023, 2046, 4092, 8184, 16368, 131072, 524288]),
            "pl_time": array([1.207858, 1.208028, 1.208212, 1.208258, 1.208252, 1.208578, 1.211231, 1.327929, 7.637243, 29.264372]),
            "ps_time": array([0.552622, 0.552698, 0.552932, 0.553015, 0.552806, 0.553265, 0.555154, 0.577049, 4.749329, 18.956692])
        }
    }
}

for test in Testbench.values():
    for impl in test.values():
        x = impl["samples"]
        f = impl["pl_time"]
        g = impl["ps_time"]

        figure()
        title(impl["name"])
        plot(x, f, '-o')
        plot(x, g, '-o')

xlabel("Samples")
ylabel("Time [ms]")

show()