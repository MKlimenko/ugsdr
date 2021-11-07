#pragma once

#include <cstdint>
#include <cstring>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>
#include <vector>

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
		Gps_L2CM,
		Gps_L5I,
		Gps_L5Q,
		GlonassCivilFdma_L1,
		GlonassCivilFdma_L2,
		Galileo_E1b,
		Galileo_E1c,
		Galileo_E5aI,
		Galileo_E5aQ,
		Galileo_E5bI,
		Galileo_E5bQ
	};

	enum class System : std::uint32_t {
		Gps = 0,
		Glonass,
		Galileo,
	};

	template <typename T>
	auto RepeatCodeNTimes(std::vector<T> code, std::size_t repeats) {
		auto code_period = code.size();
		for (std::size_t i = 1; i < repeats; ++i)
			code.insert(code.end(), code.begin(), code.begin() + code_period);

		return code;
	}

	constexpr auto GetSystemBySignal(Signal signal) {
		switch (signal) {
		case Signal::GpsCoarseAcquisition_L1:
		case Signal::Gps_L2CM:
		case Signal::Gps_L5I:
		case Signal::Gps_L5Q:
			return System::Gps;
		case Signal::GlonassCivilFdma_L1:
		case Signal::GlonassCivilFdma_L2:
			return System::Glonass;
		case Signal::Galileo_E1b:
		case Signal::Galileo_E1c:
		case Signal::Galileo_E5aI:
		case Signal::Galileo_E5aQ:
		case Signal::Galileo_E5bI:
		case Signal::Galileo_E5bQ:
			return System::Galileo;
		default:
			throw std::runtime_error("Unexpected signal");
		}
	}

	constexpr std::size_t GetCodesCount(System system) {
		switch (system) {
		case System::Gps:
			return gps_sv_count;
		case System::Glonass:
			return 1;
		case System::Galileo:
			return galileo_sv_count;
		default:
			throw std::runtime_error("Unexpected system");
		}
	}

	struct Sv {
		std::int32_t id : 8;
		System system : 8;
		Signal signal : 16;

		constexpr Sv() : Sv(0, System::Gps, Signal::GpsCoarseAcquisition_L1) {}
		constexpr Sv(std::int32_t id_val, System system_val, Signal signal_val) : id(id_val), system(system_val), signal(signal_val) {}
		constexpr Sv(std::int32_t id_val, Signal signal_val) : id(id_val), system(GetSystemBySignal(signal_val)), signal(signal_val) {}

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
			case Signal::Gps_L2CM:
				dst += "L2CM";
				break;
			case Signal::Gps_L5I:
				dst += "L5I";
				break;
			case Signal::Gps_L5Q:
				dst += "L5Q";
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
			case Signal::Galileo_E5bI:
				dst += "E5bI";
				break;
			case Signal::Galileo_E5bQ:
				dst += "E5bQ";
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
