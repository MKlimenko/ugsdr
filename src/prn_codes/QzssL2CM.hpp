#pragma once

#include "codegen.hpp"
#include "L2CM.hpp"

namespace ugsdr {
	class QzssL2CM final : public Codegen<QzssL2CM>, public L2CM {
	private:
		static auto LfsrValue(std::size_t sv_number) {
			constexpr static auto lfsr_values = std::array{
				0204244652, 0202133131, 0714351204, 0657127260, 0130567507, 0670517677, 0607275514, 0045413633, 0212645405, 0613700455,
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
