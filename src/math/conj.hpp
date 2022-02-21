#pragma once

#include "../common.hpp"

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename ConjImpl>
	class ComplexConjugate {
	protected:
	public:
		template <ComplexContainer T>
		static void Transform(T& src_dst) {
			ConjImpl::Process(src_dst);
		}

		template <ComplexContainer T>
		static auto Transform(const T& src) {
			return ConjImpl::Process(src);
		}
	};

	class SequentialConj : public ComplexConjugate<SequentialConj> {
	protected:
		friend class ComplexConjugate<SequentialConj>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst) {
			for (auto& el : src_dst)
				el = std::conj(el);
		}

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src) {
			auto dst = src;
			Process(dst);
			return dst;
		}
	};
}
