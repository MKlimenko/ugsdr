#include "cuda_mixer.cuh"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <thrust/complex.h>
#include <thrust/device_vector.h>
#include <cmath>

namespace {
	template <typename T>
	__global__ void mul_kernel(T* src_dst, double relative_frequency, double phase) {
		int i = threadIdx.x + blockIdx.x * blockDim.x;
		double triarg = 2 * 4 * std::atan(1.0) * relative_frequency * i;
		src_dst[i] *= cos(triarg);
	}

	template <typename T>
	__global__ void mul_kernel(thrust::complex<T>* src_dst, double relative_frequency, double phase) {
		int i = threadIdx.x + blockIdx.x * blockDim.x;
		double triarg = 2 * 4 * std::atan(1.0) * relative_frequency * i;
		src_dst[i] *= thrust::exp(thrust::complex<T>(0, triarg));
	}

	template <typename T>
	void ProcessDevice(thrust::device_vector<T>& src_dst, double relative_frequency, double phase) {
		cudaDeviceProp properties;
		cudaSetDevice(0);
		cudaGetDeviceProperties(&properties, 0);
		unsigned blocks_cnt = 0;
		unsigned threads_cnt = 0;

		for (std::size_t i = 1; i < src_dst.size(); ++i) {
			auto div_result = std::div(static_cast<std::ptrdiff_t>(src_dst.size()), i);
			if (div_result.rem)
				continue;
			if (div_result.quot <= properties.maxThreadsPerBlock) {
				blocks_cnt = static_cast<unsigned>(i);
				threads_cnt = static_cast<unsigned>(div_result.quot);
				break;
			}
		}

		mul_kernel<<<blocks_cnt, threads_cnt>>>(thrust::raw_pointer_cast(src_dst.data()), relative_frequency, phase);
	}
	template <typename T>
	void ProcessHost(std::vector<T>& src_dst, double relative_frequency, double phase) {
		thrust::device_vector<T> gpu(src_dst.begin(), src_dst.end());
		ProcessDevice(gpu, relative_frequency, phase);
		auto cudaStatus = cudaGetLastError();
		if (cudaStatus != cudaSuccess)
			throw std::runtime_error("Oopsie");
		thrust::copy(gpu.begin(), gpu.end(), src_dst.begin());
	}
	template <typename T>
	void ProcessHost(std::vector<std::complex<T>>& src_dst, double relative_frequency, double phase) {
		thrust::device_vector<thrust::complex<T>>gpu(src_dst.begin(), src_dst.end());
		ProcessDevice(gpu, relative_frequency, phase);
		auto cudaStatus = cudaGetLastError();
		if (cudaStatus != cudaSuccess)
			throw std::runtime_error("Oopsie");
		thrust::copy(gpu.begin(), gpu.end(), src_dst.begin());
	}
}

namespace ugsdr {
	template <>
	void CudaMixer::Process<float>(std::vector<std::complex<float>>& src_dst, double sampling_freq, double frequency, double phase) {
		ProcessHost(src_dst, frequency / sampling_freq, phase);
	}
	template <>
	void CudaMixer::Process<double>(std::vector<std::complex<double>>& src_dst, double sampling_freq, double frequency, double phase) {
		ProcessHost(src_dst, frequency / sampling_freq, phase);
	}
}
