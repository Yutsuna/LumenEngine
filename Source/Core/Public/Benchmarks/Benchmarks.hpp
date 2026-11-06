/*
- @file Benchmarks.hpp
- @brief Lumen benchmark all included.

- @code
- #include <Benchmarks/Benchmarks.hpp>

- using namespace Lumen::Benchmarks;

- // Micro: auto-calibrated, setup before the loop is not timed.
- LUMEN_BENCHMARK( Vector, PushBack )
- {
-     Lumen::TVector<int> Values;
-     Values.reserve( 1024 );
-     for ( auto _ : State )
-     {
-         Values.push_back( 42 );
-         DoNotOptimize( Values );
-         if ( Values.size() == 1024 ) { State.PauseTiming(); Values.clear(); State.ResumeTiming(); }
-     }
- }

- // Parameterized: one run per argument, with throughput.
- LUMEN_BENCHMARK_RANGE( Memory, Copy, 64, 1 << 20 )
- {
-     const Lumen::USize Size = static_cast<Lumen::USize>( State.GetArgument() );
-     Lumen::TVector<char> Source( Size ), Dest( Size );
-     State.SetBytesPerIteration( Source.size() );
-     for ( auto _ : State )
-     {
-         std::memcpy( Dest.data(), Source.data(), Source.size() );
-         ClobberMemory();
-     }
- }

- // Macro: heavy, one iteration per sample.
- LUMEN_MACRO_BENCHMARK( Assets, LoadLevel )
- {
-     for ( auto _ : State ) { /* ... */ }
-
}
- @endcode

    -
    Run: `BenchRunner[filter][results.json]` - the filter supports `*`, `?` or a plain substring on "Suite.Name".*/

#pragma once

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>
#include <Benchmarks/BenchmarkReporter.hpp>
#include <Benchmarks/BenchmarkState.hpp>
#include <Benchmarks/BenchmarkTypes.hpp>
