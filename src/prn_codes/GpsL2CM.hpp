#pragma once

#include "codegen.hpp"
#include "L2CM.hpp"

namespace ugsdr {
	class GpsL2CM final : public Codegen<GpsL2CM>, public L2CM {
	private:
		static auto LfsrValue(std::size_t sv_number) {
			constexpr static auto lfsr_values = std::array{
				0742417664, 0756014035, 0002747144, 0066265724, 0601403471, 0703232733, 0124510070, 0617316361,
				0047541621, 0733031046, 0713512145, 0024437606, 0021264003, 0230655351, 0001314400, 0222021506,
				0540264026, 0205521705, 0064022144, 0120161274, 0044023533, 0724744327, 0045743577, 0741201660,
				0700274134, 0010247261, 0713433445, 0737324162, 0311627434, 0710452007, 0722462133, 0050172213,
			};

			if (sv_number >= lfsr_values.size())
				throw std::runtime_error("SV number for the GPS L2CM code exceed available values");

			return static_cast<std::size_t>(lfsr_values[sv_number]);
		}

	protected:
		friend class Codegen<GpsL2CM>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L2CM::Generate(prn, sv_number, LfsrValue);
		}
	};
}
