#pragma once

#include <complex>
#include <mutex>
#include <numbers>
#include <numeric>
#include <vector>

#include "../helpers/is_complex.hpp"

#ifdef HAS_FFTW
#include <fftw3.h>
#include "../../external/type_map/include/type_map.hpp"
#endif

namespace ugsdr {
	template <typename DftImpl>
	class DiscreteFourierTransform {
	protected:
	public:
		// TODO: Add real-to-complex and real-to-real transforms (when required)
		template <typename UnderlyingType>
		static void Transform(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			DftImpl::Process(src_dst, is_inverse);
		}

		template <typename T>
		static void Transform(T& src_dst, bool is_inverse = false) {
			DftImpl::Process(src_dst, is_inverse);
		}
		
		template <typename UnderlyingType>
		static auto Transform(const std::vector<UnderlyingType>& src, bool is_inverse = false) {
			return DftImpl::Process(src, is_inverse);
		}
	};

	class SequentialDft : public DiscreteFourierTransform<SequentialDft> {
	private:
#ifdef HAS_FFTW
		template <typename T>
		struct FftwFunctions {
			static fftwf_plan GetFloatPlan(int n, float* in, fftwf_complex* out, int sign, unsigned flags) {
				static_cast<void>(sign);
				return fftwf_plan_dft_r2c_1d(n, in, out, flags);
			}
			static fftw_plan GetDoublePlan(int n, double* in, fftw_complex* out, int sign, unsigned flags) {
				static_cast<void>(sign);
				return fftw_plan_dft_r2c_1d(n, in, out, flags);
			}

			using CreatePlan = mk::TypeMap<
				mk::TypeValuePair<float, GetFloatPlan>,
				mk::TypeValuePair<std::complex<float>, fftwf_plan_dft_1d>,
				mk::TypeValuePair<double, GetDoublePlan> ,
				mk::TypeValuePair<std::complex<double>, fftw_plan_dft_1d>
			>;
			using Dft = mk::TypeMap<
				mk::TypeValuePair<float, fftwf_execute_dft_r2c>,
				mk::TypeValuePair<std::complex<float>, fftwf_execute_dft>,
				mk::TypeValuePair<double, fftw_execute_dft_r2c>,
				mk::TypeValuePair<std::complex<double>, fftw_execute_dft>
			>;
			using DestroyPlan = mk::TypeMap<
				mk::TypeValuePair<float, fftwf_destroy_plan>,
				mk::TypeValuePair<double, fftw_destroy_plan>
			>;
			using PlannerThreadSafe = mk::TypeMap<
				mk::TypeValuePair<float, fftwf_make_planner_thread_safe>,
				mk::TypeValuePair<double, fftw_make_planner_thread_safe>
			>;
			using DftComplexType = mk::TypeMap<
				mk::TypePair<float, float>,
				mk::TypePair<std::complex<float>, fftwf_complex>,
				mk::TypePair<double, double>,
				mk::TypePair<std::complex<double>, fftw_complex>
			>;

			static auto GetCreatePlan() {
				return CreatePlan::template GetValueByType<T>();
			}
			static auto GetDft() {
				return Dft::GetValueByType<T>();
			}
			static auto GetDestroyPlan() {
				return DestroyPlan::GetValueByType<underlying_t<T>>();
			}
			static auto GetPlannerThreadSafe() {
				return PlannerThreadSafe::GetValueByType<underlying_t<T>>();
			}
			using DftType = DftComplexType::GetTypeByType<T>;
		};
#endif

		static inline std::mutex m;

		template <typename UnderlyingType>
		static void GenerateSine(std::size_t iter, std::vector<std::complex<UnderlyingType>>& sine, bool is_inverse = false) {
			for (std::size_t j = 0; j < sine.size(); ++j) 
				sine[j] = std::exp(std::complex<UnderlyingType>(0, static_cast<UnderlyingType>((is_inverse ? 1.0 : -1.0) * 2 * j * iter * std::numbers::pi / sine.size())));
			
		}

		template <typename DstType, typename T>
		static auto ProcessImpl(const std::vector<T>& src, bool is_inverse = false) {
#ifndef HAS_FFTW
			auto imaginary_unit = DstType(0, 1);

			auto dst = std::vector<DstType>(src.size());
			auto sine = dst;
			for (std::size_t i = 0; i < src.size(); ++i) {
				GenerateSine(i, sine, is_inverse);
				dst[i] = std::inner_product(src.begin(), src.end(), sine.begin(), DstType{}) / static_cast<underlying_t<DstType>>(is_inverse ? src.size() : 1);
			}

			return dst;
#else
			auto dst = std::vector<DstType>(src.size());
			auto plan_function = FftwFunctions<T>::GetCreatePlan();
			auto src_copy = src;
			FftwFunctions<T>::GetPlannerThreadSafe()();
			auto plan = plan_function(static_cast<int>(src.size()), reinterpret_cast<typename FftwFunctions<T>::DftType*>(src_copy.data()),
				reinterpret_cast<typename FftwFunctions<DstType>::DftType*>(dst.data()), is_inverse ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);

			auto dft = FftwFunctions<T>::GetDft();
			dft(plan, reinterpret_cast<typename FftwFunctions<T>::DftType*>(src_copy.data()), reinterpret_cast<typename FftwFunctions<DstType>::DftType*>(dst.data()));

			if (is_inverse) {
				for (auto& el : dst) {
					el /= src.size();
					el *= 2;
				}
			}

			auto destroy = FftwFunctions<DstType>::GetDestroyPlan();
			destroy(plan);
		
			return dst;
#endif
		}

	protected:
		friend class DiscreteFourierTransform<SequentialDft>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			const auto& src = src_dst;
			src_dst = Process(src, is_inverse);
		}

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src, bool is_inverse = false) {
			return ProcessImpl<std::complex<UnderlyingType>>(src, is_inverse);
		}

		template <typename T>
		static auto Process(const std::vector<T>& src, bool is_inverse = false) {
			return ProcessImpl<std::complex<T>>(src, is_inverse);
		}

	public:
	};
}
