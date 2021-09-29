﻿#include <benchmark/benchmark.h>

#include <complex>
#include "../src/mixer/mixer.hpp"
#include "../src/mixer/batch_mixer.hpp"
#include "../src/mixer/ipp_mixer.hpp"

#include "../src/math/af_dft.hpp"
#include "../src/math/ipp_dft.hpp"

#include "../src/helpers/ipp_complex_type_converter.hpp"

#include "../src/matched_filter/af_matched_filter.hpp"
#include "../src/matched_filter/ipp_matched_filter.hpp"

#include "../src/correlator/af_correlator.hpp"
#include "../src/correlator/ipp_correlator.hpp"

#include "../src/helpers/af_array_proxy.hpp"

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

#if 0
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
#endif

#if 0
namespace matched_filter {
    constexpr auto max_range = 2048 << 8;

#define DFT_BENCHMARK_OPTIONS RangeMultiplier(2)->Range(2048, max_range)->Complexity()->Unit(benchmark::TimeUnit::kMicrosecond)

    template <typename T>
    static void IppMf(benchmark::State& state) {
        using MatchedFilterType = ugsdr::MatchedFilter<ugsdr::IppMatchedFilter>;

        std::vector<std::complex<T>> signal(max_range);
        std::vector<T> ir(max_range);
        MatchedFilterType::Filter(signal, ir);
        signal.resize(state.range());
        ir.resize(state.range());

        for (auto _ : state) {
            MatchedFilterType::Filter(signal, ir);
            benchmark::DoNotOptimize(signal);
            benchmark::DoNotOptimize(ir);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(IppMf, float)->DFT_BENCHMARK_OPTIONS;
    //BENCHMARK_TEMPLATE(IppMf, double)->DFT_BENCHMARK_OPTIONS;


    template <typename T>
    static void AfMf(benchmark::State& state) {
        using MatchedFilterType = ugsdr::MatchedFilter<ugsdr::AfMatchedFilter>;

        std::vector<std::complex<T>> signal(max_range);
        std::vector<T> ir(max_range);
        MatchedFilterType::Filter(signal, ir);
        signal.resize(state.range());
        ir.resize(state.range());

        for (auto _ : state) {
            MatchedFilterType::Filter(signal, ir);
            benchmark::DoNotOptimize(signal);
            benchmark::DoNotOptimize(ir);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(AfMf, float)->DFT_BENCHMARK_OPTIONS;
    BENCHMARK_TEMPLATE(AfMf, double)->DFT_BENCHMARK_OPTIONS;
}
#endif

namespace correlator {
    constexpr auto max_range = 2048 << 8;

#define CORR_BENCHMARK_OPTIONS RangeMultiplier(2)->Range(2048, max_range)->Complexity()

    template <typename T>
    static void CpuCorr(benchmark::State& state) {
        using CorrelatorType = ugsdr::Correlator<ugsdr::SequentialCorrelator>;

        std::vector<std::complex<T>> signal(state.range());
        std::vector<T> code(state.range());

        for (auto _ : state) {
            auto dst = CorrelatorType::Correlate(signal, std::span(code));
            benchmark::DoNotOptimize(dst);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(CpuCorr, float)->CORR_BENCHMARK_OPTIONS;
    //BENCHMARK_TEMPLATE(CpuCorr, double)->CORR_BENCHMARK_OPTIONS;

    template <typename T>
    static void IppCorr(benchmark::State& state) {
        using CorrelatorType = ugsdr::Correlator<ugsdr::IppCorrelator>;

        std::vector<std::complex<T>> signal(state.range());
        std::vector<T> code(state.range());

        for (auto _ : state) {
            auto dst = CorrelatorType::Correlate(signal, std::span(code));
            benchmark::DoNotOptimize(dst);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(IppCorr, float)->CORR_BENCHMARK_OPTIONS;
    //BENCHMARK_TEMPLATE(IppCorr, double)->CORR_BENCHMARK_OPTIONS;

    template <typename T>
    static void AfCorr(benchmark::State& state) {
        using CorrelatorType = ugsdr::Correlator<ugsdr::AfCorrelator>;

        const std::vector<std::complex<T>> signal_max(max_range);
        const std::vector<std::complex<T>> code_max(max_range);

        auto signal_af_max = ugsdr::ArrayProxy(signal_max);
        auto code_af_max = ugsdr::ArrayProxy(code_max);
        CorrelatorType::Correlate(signal_af_max, code_af_max);

        const std::vector<std::complex<T>> signal(state.range());
        const std::vector<std::complex<T>> code(state.range());

        auto signal_af = ugsdr::ArrayProxy(signal);
        auto code_af = ugsdr::ArrayProxy(code);

        for (auto _ : state) {
            auto dst = CorrelatorType::Correlate(signal_af, code_af);
            benchmark::DoNotOptimize(dst);
        }
        state.SetComplexityN(state.range());
    }
    BENCHMARK_TEMPLATE(AfCorr, float)->CORR_BENCHMARK_OPTIONS;
    //BENCHMARK_TEMPLATE(AfCorr, double)->CORR_BENCHMARK_OPTIONS;
}


int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    system("pause");
    return 0;
}