#pragma once

#include "fir.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"
#include "../helpers/is_complex.hpp"

#include <type_traits>

namespace ugsdr {
	template <typename T, typename WeightsContainer, typename StateContainer/* = std::vector<T>, typename StateContainer = std::vector<T>*/>
	class IppFir : public Fir<IppFir<T, WeightsContainer, StateContainer>> {
		template <typename Ty>
		struct TypeToSpec {
			using IppSpecMap = mk::TypeMap<
				mk::TypePair<float, IppsFIRSpec_32f>,
				mk::TypePair<std::complex<float>, IppsFIRSpec_32fc>,
				mk::TypePair<double, IppsFIRSpec_64f>,
				mk::TypePair<std::complex<double>, IppsFIRSpec_64fc>
			>;

			using Type = IppSpecMap::GetTypeByType<Ty>;
		};

		template <typename Ty>
		constexpr static IppDataType GetIppDataType() {
			using IppDataTypeMap = mk::TypeMap<
				mk::TypeValuePair<float, IppDataType::ipp32f>,
				mk::TypeValuePair<std::complex<float>, IppDataType::ipp32fc>,
				mk::TypeValuePair<double, IppDataType::ipp64f>,
				mk::TypeValuePair<std::complex<double>, IppDataType::ipp64fc>
			>;

			return IppDataTypeMap::GetValueByType<Ty>();
		}

		[[nodiscard]]
		static auto GetFirWrapper() {
			static auto fir_wrapper = plusifier::FunctionWrapper(
				ippsFIRSR_32f,
				ippsFIRSR_32fc,
				ippsFIRSR_64f,
				ippsFIRSR_64fc
			);

			return fir_wrapper;
		}

		using IppType = std::conditional_t<is_complex_v<T>,
			typename IppTypeToComplex<underlying_t<T>>::Type,
			T
		>;

		StateContainer state;
		WeightsContainer weights;
		std::vector<Ipp8u> buf;
		std::vector<Ipp8u> fir_spec;
		using FirSpec = typename TypeToSpec<T>::Type;

	protected:
		friend class Fir<IppFir<T, WeightsContainer, StateContainer>>;

		template <typename Ty>
		constexpr static void VerifyType() {
			static_assert(std::is_same_v<Ty, T> || std::is_same_v<std::complex<Ty>, T>, "Please use matching filter weights and input data");
		}

		template <typename Ty>
		void Process(std::vector<Ty>& src_dst) {
			src_dst = Process(const_cast<const std::vector<Ty>&>(src_dst));
		}

		template <typename Ty>
		[[nodiscard]]
		auto Process(const std::vector<Ty>& src) {
			VerifyType<Ty>();
			auto fir = GetFirWrapper();
			auto dst = src;
			fir(reinterpret_cast<const IppType*>(src.data()), reinterpret_cast<IppType*>(dst.data()),
				static_cast<int>(src.size()), reinterpret_cast<FirSpec*>(fir_spec.data()),
				reinterpret_cast<IppType*>(state.data()), reinterpret_cast<IppType*>(state.data()), buf.data());
			return dst;
		}
		
		[[nodiscard]]
		static auto GetFirInitWrapper() {
			static auto firinit_wrapper = plusifier::FunctionWrapper(
				ippsFIRSRInit_32f,
				ippsFIRSRInit_32fc,
				ippsFIRSRInit_64f,
				ippsFIRSRInit_64fc
			);

			return firinit_wrapper;
		}

		void Update(const WeightsContainer& filter_coefficients) {
			weights = filter_coefficients;
			auto fir_init = GetFirInitWrapper();
			fir_init(reinterpret_cast<const IppType*>(weights.data()), static_cast<int>(weights.size()), ippAlgAuto,
				reinterpret_cast<FirSpec*>(fir_spec.data()));
		}

		void Init() {
			int spec_size = 0;
			int buf_size = 0;
			ippsFIRSRGetSize(static_cast<int>(weights.size()), GetIppDataType<T>(), &spec_size, &buf_size);
			fir_spec.resize(spec_size);
			buf.resize(buf_size);

			auto fir_init = GetFirInitWrapper();
			fir_init(reinterpret_cast<const IppType*>(weights.data()), static_cast<int>(weights.size()), ippAlgAuto,
				reinterpret_cast<FirSpec*>(fir_spec.data()));			
		}

		[[nodiscard]]
		const auto& Weights() const {
			return weights;
		}

	public:
		IppFir(std::vector<T> inp_weights) : state(inp_weights.size()), weights(std::move(inp_weights)) {
			Init();
		}
		IppFir(std::size_t weights_size) : state(weights_size), weights(weights_size) {
			Init();
		}
	};

	template<typename T>
	IppFir(std::vector<T> weights)->IppFir<T, std::vector<T>, std::vector<T>>;
	template<typename T>
	IppFir(std::vector<std::complex<T>> weights)->IppFir<std::complex<T>, std::vector<std::complex<T>>, std::vector<std::complex<T>>>;

}
#endif