#pragma once

#include <cstdint>
#include <cstring>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

#ifdef HAS_SIGNAL_PLOT
#define NOMINMAX
#include "signalsviewermdi.hpp"
#endif

namespace ugsdr {
	constexpr std::size_t gps_sv_count = 32;
	constexpr std::int32_t gln_min_frequency = -7;
	constexpr std::int32_t gln_max_frequency = 6;
	constexpr std::size_t galileo_sv_count = 50;

	template <typename T>
	void CheckResize(T& vec, std::size_t samples) {
		if (vec.size() != samples)
			vec.resize(samples);
	}
	
	enum class Signal : std::uint32_t {
		GpsCoarseAcquisition_L1,
		GlonassCivilFdma_L1,
		GlonassCivilFdma_L2,
		Galileo_E1b,
		Galileo_E1c,
		Galileo_E5aI,
		Galileo_E5aQ
	};

	enum class System : std::uint32_t {
		Gps = 0,
		Glonass,
		Galileo,
	};

	struct Sv {
		std::int32_t id : 8;
		System system : 8;
		Signal signal : 16;

		constexpr Sv() : Sv(0, System::Gps, Signal::GpsCoarseAcquisition_L1) {}
		constexpr Sv(std::int32_t id_val, System system_val, Signal signal_val) : id(id_val), system(system_val), signal(signal_val) {}

		operator Signal() const {
			return signal;
		}
		
		operator System() const {
			return system;
		}
		
		operator std::int32_t() const {
			return id;
		}

		explicit operator std::uint32_t() const {
			return *reinterpret_cast<const std::uint32_t*>(this);
		}

		operator std::string() const {
			std::string dst;
			auto sv_number = id;
			switch (system) {
			case System::Gps:
				dst += "GPS SV";
				++sv_number;
				break;
			case System::Glonass:
				dst += "Glonass litera ";
				break;
			case System::Galileo:
				dst += "Galileo SV";
				++sv_number;
				break;
			default:
				break;
			}
			dst += std::to_string(sv_number) + ". ";
			switch (signal) {
			case Signal::GpsCoarseAcquisition_L1:
				dst += "C/A L1";
				break;
			case Signal::GlonassCivilFdma_L1:
				dst += "L1OF";
				break;
			case Signal::GlonassCivilFdma_L2:
				dst += "L2OF";
				break;
			case Signal::Galileo_E1b:
				dst += "E1B";
				break;
			case Signal::Galileo_E1c:
				dst += "E1C";
				break;
			case Signal::Galileo_E5aI:
				dst += "E5aI";
				break;
			case Signal::Galileo_E5aQ:
				dst += "E5aQ";
				break;
			default:
				break;
			}

			return dst;
		}

		operator std::wstring() const {
			auto str = static_cast<std::string>(*this);
			
			return std::wstring(str.begin(), str.end());
		}
		
		bool operator<(const Sv& rhs) const {
			if (system != rhs.system)
				return static_cast<std::uint32_t>(system) < static_cast<std::uint32_t>(rhs.system);
			if (signal != rhs.signal)
				return static_cast<std::uint32_t>(signal) < static_cast<std::uint32_t>(rhs.signal);
			return id < rhs.id;
		}

		template <class Archive>
		void save(Archive& ar) const {
			ar(static_cast<std::uint32_t>(*this));
		}

		template <class Archive>
		void load(Archive& ar) {
			std::uint32_t val = 0;
			ar(val);
			*reinterpret_cast<std::uint32_t*>(this) = val;
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
