#include <benchmark/benchmark.h>

#include <complex>
#include "../src/mixer/mixer.hpp"

static void MixerSequential(benchmark::State& state) {
    std::vector<std::complex<double>> input(state.range());
    for (auto _ : state) {
        ugsdr::Mixer::TranslateSequential(input, 100.0, 1.0);
        benchmark::DoNotOptimize(input);
    }
    state.SetComplexityN(state.range());
}
BENCHMARK(MixerSequential)->RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity();


static void MixerParallel(benchmark::State& state) {
    std::vector<std::complex<double>> input(state.range());
    for (auto _ : state) {
        ugsdr::Mixer::Translate(input, 100.0, 1.0);
        benchmark::DoNotOptimize(input);
    }
    state.SetComplexityN(state.range());
}
BENCHMARK(MixerParallel)->RangeMultiplier(2)->Range(2048, 2048 << 6)->Complexity();

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    system("pause");
    return 0;
}