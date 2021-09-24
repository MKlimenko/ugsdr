#include <benchmark/benchmark.h>

#include <complex>
#include "../src/mixer/mixer.hpp"
#include "../src/mixer/batch_mixer.hpp"
#include "../src/mixer/ipp_mixer.hpp"

#include "../src/math/af_dft.hpp"
#include "../src/math/ipp_dft.hpp"
#include "../src/helpers/ipp_complex_type_converter.hpp"

#include <execution>
#include <random>

#include "ipp.h"

#if 0
namespace mixer {
#define MIXER_BENCHMARK_OPTIONS RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity()

    template <typename T>
    static void MixerSequential(benchmark::State& state) {
        std::vector<std::complex<T>> input(state.range());
        for (auto _ : state) {
            ugsdr::SequentialMixer::Translate(input, 100.0, 1.0);
            benchmark::DoNotOptimize(input);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(MixerSequential, std::int8_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerSequential, std::int16_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerSequential, std::int32_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerSequential, float)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerSequential, double)->MIXER_BENCHMARK_OPTIONS;

    template <typename T>
    static void MixerParallel(benchmark::State& state) {
        std::vector<std::complex<T>> input(state.range());
        for (auto _ : state) {
            ugsdr::BatchMixer::Translate(input, 100.0, 1.0);
            benchmark::DoNotOptimize(input);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(MixerParallel, std::int8_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerParallel, std::int16_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerParallel, std::int32_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerParallel, float)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerParallel, double)->MIXER_BENCHMARK_OPTIONS;

    template <typename T>
    static void MixerIpp(benchmark::State& state) {
        std::vector<std::complex<T>> input(state.range());
        for (auto _ : state) {
            ugsdr::IppMixer::Translate(input, 100.0, 1.0);
            benchmark::DoNotOptimize(input);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(MixerIpp, std::int8_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerIpp, std::int16_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerIpp, std::int32_t)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerIpp, float)->MIXER_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(MixerIpp, double)->MIXER_BENCHMARK_OPTIONS;
}
#endif

#if 0
namespace ipp_math {
	static void Multiply(benchmark::State& state) {
	    std::vector<Ipp32fc> a(state.range());
	    std::vector<Ipp32fc> b(state.range());
	    std::vector<Ipp32fc> c(state.range());
	    for (auto _ : state) {
	        ippsMul_32fc(a.data(), b.data(), c.data(), static_cast<int>(a.size()));
	        benchmark::DoNotOptimize(a);
	        benchmark::DoNotOptimize(b);
	        benchmark::DoNotOptimize(c);
	    }
	    state.SetComplexityN(state.range());
	}
	BENCHMARK(Multiply)->RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity();

	static void ConjPlusMultiply(benchmark::State& state) {
	    std::vector<Ipp32fc> a(state.range());
	    std::vector<Ipp32fc> b(state.range());
	    std::vector<Ipp32fc> c(state.range());
	    for (auto _ : state) {
	        ippsConj_32fc_I(a.data(), static_cast<int>(a.size()));
	        ippsMul_32fc(a.data(), b.data(), c.data(), static_cast<int>(a.size()));
	        benchmark::DoNotOptimize(a);
	        benchmark::DoNotOptimize(b);
	        benchmark::DoNotOptimize(c);
	    }
	    state.SetComplexityN(state.range());
	}
	BENCHMARK(ConjPlusMultiply)->RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity();


	static void MultiplyByConj(benchmark::State& state) {
	    std::vector<Ipp32fc> a(state.range());
	    std::vector<Ipp32fc> b(state.range());
	    std::vector<Ipp32fc> c(state.range());
	    for (auto _ : state) {
	        ippsMulByConj_32fc_A24(a.data(), b.data(), c.data(), static_cast<int>(a.size()));
	        benchmark::DoNotOptimize(a);
	        benchmark::DoNotOptimize(b);
	        benchmark::DoNotOptimize(c);
	    }
	    state.SetComplexityN(state.range());
	}
	BENCHMARK(MultiplyByConj)->RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity();
}
#endif

namespace dft {
    constexpr auto max_range = 2048 << 8;
	
#define DFT_BENCHMARK_OPTIONS RangeMultiplier(2)->Range(2048, max_range)->Complexity()
	
    template <typename T>
    static void Ipp(benchmark::State& state) {
        using DftType = ugsdr::DiscreteFourierTransform<ugsdr::IppDft>;

        std::vector<std::complex<T>> vec(max_range);
        DftType::Transform(vec);
        vec.resize(state.range());

        for (auto _ : state) {
            DftType::Transform(vec);
            benchmark::DoNotOptimize(vec);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(Ipp, float)->DFT_BENCHMARK_OPTIONS;
    //BENCHMARK_TEMPLATE(Ipp, double)->DFT_BENCHMARK_OPTIONS;


    template <typename T>
    static void Af(benchmark::State& state) {
        using DftType = ugsdr::DiscreteFourierTransform<ugsdr::AfDft>;

        std::vector<std::complex<T>> vec(max_range);
        DftType::Transform(vec);
        vec.resize(state.range());

        for (auto _ : state) {
            DftType::Transform(vec);
            benchmark::DoNotOptimize(vec);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(Af, float)->DFT_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(Af, double)->DFT_BENCHMARK_OPTIONS;

}

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    system("pause");
    return 0;
}