#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class QzssL5I final : public Codegen<QzssL5I>, public L5 {
	private:
		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				5836, 926, 6086, 950, 5905, 3240, 6675, 3197, 1555, 3589
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the QZSS L5I code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<QzssL5I>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
