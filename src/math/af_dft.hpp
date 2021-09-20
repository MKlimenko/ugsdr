#pragma once

#include "dft.hpp"

#include <type_traits>

namespace ugsdr {
	class AfDft : public DiscreteFourierTransform<AfDft> {
	private:
	protected:
		friend class DiscreteFourierTransform<AfDft>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {

		}

		template <typename UnderlyingType>
		static auto Process(const std::vector<UnderlyingType>& src, bool is_inverse = false) {
			
			return src;
		}
	};
}
