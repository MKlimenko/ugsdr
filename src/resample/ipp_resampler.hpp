#pragma once

#include "ipp.h"
#include "resampler.hpp"
#include "ipp_upsampler.hpp"
#include "ipp_decimator.hpp"
#include "../../external/plusifier/Plusifier.hpp"
#include "../../external/type_map/include/type_map.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include <complex>
#include <memory>
#include <numeric>

namespace ugsdr {
	template <bool use_filter = false>
	class IppResamplerBase : public Resampler<IppResamplerBase<use_filter>> {
	private:
		constexpr static inline int taps_len = 50;

		static auto GetBufferSize() {
			int buffer_size = 0;
			ippsFIRGenGetBufferSize(taps_len, &buffer_size);
			return static_cast<std::size_t>(buffer_size);
		}

		template <typename TypeToCast>
		static auto GenerateFirCoefficients(std::size_t new_sampling_rate, std::size_t lcm) {
			auto buffer_size = GetBufferSize();
			auto buffer = std::make_unique<Ipp8u[]>(buffer_size);
			auto cutoff = static_cast<double>(new_sampling_rate) / lcm;
			std::vector<double> taps(taps_len);
			ippsFIRGenLowpass_64f(cutoff, taps.data(), taps_len, ippWinRect, IppBool::ippTrue, buffer.get());
			ippsWinKaiser_64f_I(taps.data(), taps_len, 5);

			Add(L"FIR", &taps[0], taps_len);

			std::vector<TypeToCast> dst;
			dst.reserve(taps_len);
			using UnderlyingIppType = typename IppComplexToType<TypeToCast>::Type;

			for (auto& el : taps)
				dst.push_back(static_cast<TypeToCast>(static_cast<UnderlyingIppType>(el)));
			return dst;
		}

		static auto GetFirInitWrapper() {
			static auto fir_wrapper = plusifier::FunctionWrapper(
				ippsFIRSRInit_32f, ippsFIRSRInit_32fc, ippsFIRSRInit_64f, ippsFIRSRInit_64fc
			);

			return fir_wrapper;
		}
		static auto GetFirWrapper() {
			static auto fir_wrapper = plusifier::FunctionWrapper(
				ippsFIRSR_32f, ippsFIRSR_32fc, ippsFIRSR_64f, ippsFIRSR_64fc
			);

			return fir_wrapper;
		}

		template <typename T>
		constexpr static IppDataType GetIppDataType() {
			using IppDataTypeMap = mk::TypeMap<
				mk::TypeValuePair<Ipp32f,  IppDataType::ipp32f>,
				mk::TypeValuePair<Ipp32fc, IppDataType::ipp32fc>,
				mk::TypeValuePair<Ipp64f,  IppDataType::ipp64f>,
				mk::TypeValuePair<Ipp64fc, IppDataType::ipp64fc>
			>;

			return IppDataTypeMap::GetValueByType<T>();
		}

		template <typename T>
		constexpr static auto GetFirSpec(Ipp8u* spec_buf) {
			using FirSpecMap = mk::TypeMap<
				mk::TypePair<Ipp32f,  IppsFIRSpec_32f>,
				mk::TypePair<Ipp32fc, IppsFIRSpec_32fc>,
				mk::TypePair<Ipp64f,  IppsFIRSpec_64f>,
				mk::TypePair<Ipp64fc, IppsFIRSpec_64fc>
			>;
		
			return reinterpret_cast<FirSpecMap::GetTypeByType<T>*>(spec_buf);
		}
		
		template <typename TypeToCast, typename T>
		static void FilterImpl(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate, std::size_t lcm) {
			const auto fir = GenerateFirCoefficients<TypeToCast>(new_sampling_rate, lcm);

			int spec_size = 0;
			int buf_size = 0;
			ippsFIRSRGetSize(taps_len, GetIppDataType<TypeToCast>(), &spec_size, &buf_size);
			
			std::vector<Ipp8u> spec(spec_size);
			std::vector<Ipp8u> buf(buf_size);
			std::vector<TypeToCast> delay(taps_len);
			auto spec_ptr = GetFirSpec<TypeToCast>(spec.data());
			auto fir_init_wrapper = GetFirInitWrapper();
			fir_init_wrapper(fir.data(), taps_len, IppAlgType::ippAlgAuto, spec_ptr);
			
			auto fir_wrapper = GetFirWrapper();
			src_dst.resize(src_dst.size() + taps_len);
			fir_wrapper(reinterpret_cast<TypeToCast*>(src_dst.data()), reinterpret_cast<TypeToCast*>(src_dst.data()),
				static_cast<int>(src_dst.size()), spec_ptr, nullptr, delay.data(), buf.data());
			src_dst.erase(src_dst.begin(), src_dst.begin() + taps_len);
		}

		template <typename T>
		static void Filter(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate, std::size_t lcm) {
			FilterImpl<T>(src_dst, new_sampling_rate, old_sampling_rate, lcm);
		}

		template <typename UnderlyingType>
		static void Filter(std::vector<std::complex<UnderlyingType>>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate, std::size_t lcm) {
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			FilterImpl<IppType>(src_dst, new_sampling_rate, old_sampling_rate, lcm);
		}

		using UpsamplerType = Upsampler<IppUpsampler>;
		using DecimatorType = Decimator<IppAccumulator>;

	protected:
		friend class Resampler<IppResamplerBase<use_filter>>;

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto lcm = std::lcm(new_sampling_rate, old_sampling_rate);
			auto new_samples = lcm * src_dst.size() / old_sampling_rate;

			UpsamplerType::Transform(src_dst, new_samples);
			if constexpr (use_filter)
				Filter(src_dst, new_sampling_rate, old_sampling_rate, lcm);
			DecimatorType::Transform(src_dst, lcm / new_sampling_rate);
		}

	public:
	};

	using IppResampler = IppResamplerBase<false> ;
}
