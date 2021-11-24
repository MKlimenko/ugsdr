#pragma once

#include "../common.hpp"

#include <cstring>
#include <execution>
#include <vector>

namespace ugsdr {
	class TimeScale final {
	private:
		std::vector<double> time_ms;
		std::vector<double> system_time_ms;
		bool is_corrected = false;
		std::size_t last_preable_position = 0;		

	public:
		TimeScale(std::size_t ms_cnt = 0) : time_ms(ms_cnt), system_time_ms(ms_cnt) {
			std::iota(time_ms.begin(), time_ms.end(), 0.0);
		}

		void UpdateScale(std::size_t preable_position, std::size_t tow_ms, System system) {
			if (is_corrected && system == System::Glonass)
				return;

			if (is_corrected && last_preable_position < preable_position)
				return;

			auto initial_ms_offset = 70;
			double current_offset = tow_ms + initial_ms_offset - preable_position;
			std::iota(time_ms.begin(), time_ms.end(), current_offset);
			std::transform(std::execution::par_unseq, time_ms.begin(), time_ms.end(), system_time_ms.begin(),
				[initial_ms_offset](auto& val) {return val - initial_ms_offset + 2; });
			is_corrected = true;
			last_preable_position = preable_position;
		}

		auto operator[](std::size_t ms_cnt) const {
			return time_ms[ms_cnt];
		}

		auto first() const {
			return time_ms.front();
		}

		auto last() const {
			return time_ms.back();
		}

		auto length() const {
			return time_ms.size();
		}
	};
}
