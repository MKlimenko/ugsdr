#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class QzssL5Q final : public Codegen<QzssL5Q>, public L5 {
	private:
		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				4757,	427,	5452,	5182,	6606,	6531,	4268,	3115,	6835,	862
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the QZSS L5Q code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<QzssL5Q>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
