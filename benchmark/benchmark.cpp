#include <benchmark/benchmark.h>

static void BM_SomeFunction(benchmark::State& state) {
    for (auto _ : state) {
    }
}
BENCHMARK(BM_SomeFunction);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    system("pause");
    return 0;
}