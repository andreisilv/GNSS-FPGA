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
        "luts": 507, "registers": 739,"ram_percent": 2.10, "dsp": 8,
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
        "hls_estimation": 8.750,
        "latency": 15 , "axi": "stream",
        "io": {"in_bus": 32, "out_bus": 128},
        "hls_directives": {
            "loop unroll": 2, "unroll factor": 2, "pipeline": False, "loop flatten": False
        }
    }
}

Testbench = {
    # performance in miliseconds
    # (ps time corresponds to the performance of the accelerated code section in the processor, for comparison)

    "tracking correlator": {
        "macc_v2": {
            "name": "Correlation and Integration",
            "samples": array([127, 255, 511, 1023, 2046, 4092, 8184, 16368]),
            "pl_time": array([0.300126, 0.314160, 0.343391, 0.400929, 0.517458, 0.749031, 1.203052, 2.166640]),
            "ps_time": array([0.009828, 0.018825, 0.038471, 0.077458, 0.155917, 0.312169, 0.621526, 1.246951])
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