from numpy import array
from matplotlib.pyplot import scatter, show, xlabel, ylabel

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
    "dma_axi_reset" : {"luts": 4475-(507+290), "register": 5793-(739+289), "ram_percent": 19.17-(2.10+0.82), "dsp": 0},
    "ip": {
        "macc_v2": {
            "luts": 507+290, "registers": 739+289,"ram_percent": 2.10+0.82, "dsp": 8,
            "target_clock": 10, "estimated_clock": 6.914, "pl_WNS": 0.488,
            "latency": 13, "thoughput": -1, "axi": "stream",
            "additional_blocks": ["Floating Point (7.1)"]
        }
    }
}

'''
    Testbench performance in miliseconds
    (ps time corresponds to the performance of the accelerated code section in the processor, for comparison)
'''
Testbench = {
    "Tracking Correlator" : {
        "macc_v2": array ([
            [127, 0.300126],
            [255, 0.314160],
            [511, 0.343391],
            [1023, 0.400929],
            [2046, 0.517458],
            [4092, 0.749031],
            [8184, 1.203052],
            [16368, 2.166640]
        ]),
        "ps" : array([
            [127, 0.009828],
            [255, 0.018825],
            [511, 0.038471],
            [1023, 0.077458],
            [2046, 0.155917],
            [4092, 0.312169],
            [8184, 0.621526],
            [16368, 1.246951]
        ])
    }
}

x = Testbench["Tracking Correlator"]["ps"][:,0]
ps = Testbench["Tracking Correlator"]["ps"][:,1]
macc = Testbench["Tracking Correlator"]["macc_v2"][:,1]

scatter(x, ps)
scatter(x, macc)

xlabel("#samples")
ylabel("Time [ms]")

show()