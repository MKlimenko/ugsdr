#pragma once

#include "codegen.hpp"
#include "L2CM.hpp"

namespace ugsdr {
	class QzssL2CM final : public Codegen<QzssL2CM>, public L2CM {
	private:
		static auto LfsrValue(std::size_t sv_number) {
			constexpr static auto lfsr_values = std::array{
				46417299,	69649373,	20103862,	6512359,	129712484,	122985405,	28334765,	105513904,	98275890,	90363434,
			};

			if (sv_number >= lfsr_values.size())
				throw std::runtime_error("SV number for the QZSS L2CM code exceed available values");

			return static_cast<std::size_t>(lfsr_values[sv_number]);
		}

	protected:
		friend class Codegen<QzssL2CM>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L2CM::Generate(prn, sv_number, LfsrValue);
		}
	};
}
