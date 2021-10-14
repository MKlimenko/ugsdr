#pragma once

#include <cstdint>
#include <cstring>

#ifdef HAS_SIGNAL_PLOT
#define NOMINMAX
#include "signalsviewermdi.hpp"
#endif

namespace ugsdr {
	constexpr std::size_t gps_sv_count = 32;
	constexpr std::int32_t gln_min_frequency = -7;
	constexpr std::int32_t gln_max_frequency = 6;

	enum class Signal : std::uint32_t {
		GpsCoarseAcquisition_L1,
		GlonassCivilFdma_L1,
	};

	enum class System : std::uint32_t {
		Gps = 0,
		Glonass,
	};

	struct Sv {
		std::int32_t id : 8;
		System system : 24;
				
		operator System() const {
			return system;
		}
		
		operator std::int32_t() const {
			return id;
		}

		explicit operator std::uint32_t() const {
			return *reinterpret_cast<const std::uint32_t*>(this);
		}

		bool operator<(const Sv& rhs) const {
			if (system != rhs.system)
				return static_cast<std::uint32_t>(system) < static_cast<std::uint32_t>(rhs.system);
			return id < rhs.id;
		}
	};

#ifdef HAS_SIGNAL_PLOT
	inline CSignalsViewer* glob_sv = nullptr;

	template <typename T, typename ...Args>
	void Add(const std::vector<T>& vec, Args&&... args) {
		static std::size_t signal_cnt = 0;
		glob_sv->Add(std::to_wstring(signal_cnt++), vec, std::forward<Args>(args)...);
	}
	template <typename ...Args>
	void Add(const std::wstring& signal_name, Args&&... args) {
		glob_sv->Add(signal_name, std::forward<Args>(args)...);
	}
#else
	template <typename ...Args>
	void Add(Args&&... args) {}
#endif
}
