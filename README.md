# Universal GNSS Software-defined Receiver (UGSDR)

This is a little pet project I've been working on during weekends and after-work hours, designed to be a research testbench for GNSS (Global Navigation Satellite Systems) software-defined receivers. Currently, it consists of a post-processing receiver (IF- or RF-sampled data processing from correlators to positioning), benchmark subproject and some tests.

- [Universal GNSS Software-defined Receiver (UGSDR)](#universal-gnss-software-defined-receiver-ugsdr)
  - [Tracking table](#tracking-table)
  - [Build requirements](#build-requirements)
    - [Mandatory dependencies](#mandatory-dependencies)
    - [Optional dependencies](#optional-dependencies)
  - [Roadmap](#roadmap)

## Tracking table

I've supported most of the currently used civil GNSS signals, BeiDou implementation lags behind. I've omitted the L2CL (long) code for now, as well as the military signals. There is a nice niche for research of various codeless tracking methods (Z-tracking and so on), so feel free to contribute.

There's a little table with supported signals, `Tracking` means there's a valid code generator, acquisition and tracking algorithms, but it can't (for now) be used for positioning for various reasons (no ephemeris decoding is the most common reason). `Observables` indicate that this is a derived signal, so the millisecond pseudorange ambiguity is resolved, but the navigation message is not used. `Positioning` stands for complete processing, including ephemeris decoding and millisecond ambiguity resolution. `Work in progress` speaks for itself.

|        	| GPS         	| GLONASS     	| Galileo  	| BeiDou           	| NavIC    	| SBAS     	| QZSS     	|
|--------	|-------------	|-------------	|----------	|------------------	|----------	|----------	|----------	|
| L1 C/A 	| Positioning 	|             	|          	|                  	|          	| Tracking 	| Tracking 	|
| L1OF   	|             	| Positioning 	|          	|                  	|          	|          	|          	|
| E1B    	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E1C    	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| B1I    	|             	|             	|          	|     Tracking     	|          	|          	|          	|
| B1C    	|             	|             	|          	| Work in progress 	|          	|          	|          	|
| L1S    	|             	|             	|          	|                  	|          	|          	| Tracking 	|
| L2CM   	| Observables 	|             	|          	|                  	|          	|          	| Tracking 	|
| L2OF   	|             	| Observables 	|          	|                  	|          	|          	|          	|
| L5I    	| Observables 	|             	|          	|                  	|          	| Tracking 	| Tracking 	|
| L5Q    	| Observables 	|             	|          	|                  	|          	| Tracking 	| Tracking 	|
| L5 C/A 	|             	|             	|          	|                  	| Tracking 	|          	|          	|
| E5a-I  	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E5a-Q  	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E5b-I  	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E5b-Q  	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E6b    	|             	|             	| Tracking 	|                  	|          	|          	|          	|
| E6c    	|             	|             	| Tracking 	|                  	|          	|          	|          	|

## Build requirements

I've tried to make this project as cross-platform and as easy to build as possible. Since it's a pet project, I also wanted to use some of the libraries I've always wanted to, as well as some new and shiny C++ features. There are mandatory and optional dependencies

### Mandatory dependencies

Some of the following requirements may be loosened in the future to provide a more flexible build:

1. C++ compiler with C++20 support. I've used `std::span` quite heavily, so this probably won't be relaxed. There are also some nice additions like `std::numbers` for π or `std::ranges` and `operator<=>`, but they are not so necessary for the performance as a non-owning data view. Specific compiler versions would be specified later.
2. CMake 3.18+. I just love CMake, I've worked so much with various build systems and approaches to appreciate the CMake.
3. Subset of the `Boost` library suite. There is a significant performance boost (pun intended) when using memory-mapped I/O instead of the traditional `ifstream::read()` approach. I personally also love the console progress bar, which is very handy for console-only applications.
4. My own `type_map` and `plusifier` libraries, but they're checked out as submodules, so you don't have to worry about them.
5. `RTKLIB` is used for positioning (temporarily) and RINEX output. It's being downloaded via `FetchContent` at configuration time.
6. `FFTW3` library. According to the [convolution theorem](https://en.wikipedia.org/wiki/Convolution_theorem), circular convolution may be substituted by the elementwise multiplication of the Fourier transforms (plain with conjugated to be precise) of the input signals. Circular convolution is the main operation of the matched filter, used heavily in the acquisition routines, this provides a significant boost against the plain implementation. The benefit is so great that the plain solution is removed completely.

### Optional dependencies

1. `ArrayFire` library for GPU-based computing. It was a promising ride and I've implemented multiple operations with it, but the main problem with it is that to get the best performance, you have to use the GPU-centered approach when you have a big chunk of data (1 millisecond of samples for example) and you upload it to the GPU so that the whole processing is being performed at the device. This would require some architectural modification of the UGSDR, so it just waits there. I've tested it on both GTX 1060 6GB and RTX 2080 Ti.
2. `cereal` library for serialization. This is the first project I've used this library in, it shines when there's a need to save the intermediate data, but according to clang's `-ftime-trace` it has a serious compilation time impact.
3. `gcem` is a great compile-time math library, used in the generation of the sin-cos table for the NCO.
4. `IPP`. This is the best and the most performant digital signal processing library out there for the x86 and I love it. The only downside for me is that there's no C++ interface, so I've used my `plusifier` library to abstract away the typed functions. 
5. `GTest` for testing.
6. `googlebenchmark` for benchmarking. These are the primary candidates to be moved to the optional section.

## Roadmap

There's still a lot to do, but some of the major steps are the following:

- [ ] Add the single-pass receiver (realtime-friendly)
- [ ] Add the missing (B1C, B2I etc) and perspective (L1C, L1OC etc) signals 
- [ ] Improve the acquisition (add fine acquisition step and bit boundary detection)
- [ ] Improve the tracking (strobe correlator, vision correlator, VPLL etc)
- [ ] Add the missing ephemeris decoding
- [ ] Add high-precision positioning modes (RTK, PPP etc)
- [ ] Add sensor fusion approaches (GNSS/IMU, GNSS/Video etc)
- [ ] Add specialized positioning modes (Snapshot positioning)
- [ ] Add antijamming and antispoofing modes
- [ ] TBD
