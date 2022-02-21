#pragma once

#include <complex>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef HAS_SIGNAL_PLOT
#include "signalsviewermdi.hpp"
#endif

#ifdef HAS_ARRAYFIRE
#include "helpers/af_array_proxy.hpp"
#endif

namespace ugsdr {
	constexpr std::size_t gps_sv_count = 32;
	constexpr std::int32_t gln_min_frequency = -7;
	constexpr std::int32_t gln_max_frequency = 6;
	constexpr std::size_t galileo_sv_count = 50;
	constexpr std::size_t beidou_sv_count = 63;
	constexpr std::size_t navic_sv_count = 14;
	constexpr std::size_t sbas_sv_count = 22;
	constexpr std::size_t sbas_sv_offset = 120;
	constexpr std::size_t qzss_sv_count = 10;

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
		Galileo_E5bQ,
		Galileo_E6b,
		Galileo_E6c,
		BeiDou_B1I,
		BeiDou_B1C,
		NavIC_L5,
		SbasCoarseAcquisition_L1,
		Sbas_L5I,
		Sbas_L5Q,
		QzssCoarseAcquisition_L1,
		Qzss_L1S,
		Qzss_L2CM,
		Qzss_L5I,
		Qzss_L5Q,
	};

	enum class System : std::uint32_t {
		Gps = 0,
		Glonass,
		Galileo,
		BeiDou,
		NavIC,
		Sbas,		// add specializations (WAAS, EGNOS, SDCM etc) if required 
		Qzss,
	};

	template <typename T>
	auto RepeatCodeNTimes(std::vector<T> code, std::size_t repeats) {
		auto code_period = static_cast<std::ptrdiff_t>(code.size());
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
		case Signal::Galileo_E6b:
		case Signal::Galileo_E6c:
			return System::Galileo;
		case Signal::BeiDou_B1I:
		case Signal::BeiDou_B1C:
			return System::BeiDou;
		case Signal::NavIC_L5:
			return System::NavIC;
		case Signal::SbasCoarseAcquisition_L1:
		case Signal::Sbas_L5I:
		case Signal::Sbas_L5Q:
			return System::Sbas;
		case Signal::QzssCoarseAcquisition_L1:
		case Signal::Qzss_L1S:
		case Signal::Qzss_L2CM:
		case Signal::Qzss_L5I:
		case Signal::Qzss_L5Q:
			return System::Qzss;
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
		case System::BeiDou:
			return beidou_sv_count;
		case System::NavIC:
			return navic_sv_count;
		case System::Sbas:
			return sbas_sv_count;
		case System::Qzss:
			return qzss_sv_count;
		default:
			throw std::runtime_error("Unexpected system");
		}
	}

	struct Sv {
		std::int32_t id : 16;
		System system : 8;
		Signal signal : 8;

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
			case System::BeiDou:
				dst += "BeiDou SV";
				++sv_number;
				break;
			case System::NavIC:
				dst += "NavIC SV";
				++sv_number;
				break;
			case System::Sbas:
				dst += "SBAS SV";
				++sv_number;
				break;
			case System::Qzss:
				dst += "QZSS SV";
				++sv_number;
				break;
			default:
				throw std::runtime_error("Unexpected system");
			}
			dst += std::to_string(sv_number) + ". ";
			switch (signal) {
			case Signal::GpsCoarseAcquisition_L1:
			case Signal::SbasCoarseAcquisition_L1:
			case Signal::QzssCoarseAcquisition_L1:
				dst += "C/A L1";
				break;
			case Signal::Gps_L2CM:
			case Signal::Qzss_L2CM:
				dst += "L2CM";
				break;
			case Signal::Gps_L5I:
			case Signal::Qzss_L5I:
			case Signal::Sbas_L5I:
				dst += "L5I";
				break;
			case Signal::Gps_L5Q:
			case Signal::Qzss_L5Q:
			case Signal::Sbas_L5Q:
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
			case Signal::Galileo_E6b:
				dst += "E6B";
				break;
			case Signal::Galileo_E6c:
				dst += "E6C";
				break;
			case Signal::BeiDou_B1I:
				dst += "B1I";
				break;
			case Signal::BeiDou_B1C:
				dst += "B1C";
				break;
			case Signal::NavIC_L5:
				dst += "L5 C/A";
				break;
			case Signal::Qzss_L1S:
				dst += "L1 SAIF";
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

	template <typename T>
	struct IsContainer: std::false_type {};
	template <typename T>
	struct IsContainer<std::vector<T>> : std::true_type {};
	template <typename T>
	struct IsContainer<std::span<T>> : std::true_type {};

	template <typename T>
	struct IsComplexContainer : std::false_type {};
	template <typename T>
	struct IsComplexContainer<std::vector<std::complex<T>>> : std::true_type {};
	template <typename T>
	struct IsComplexContainer<std::span<std::complex<T>>> : std::true_type {};

	template <typename T>
	concept Container = IsContainer<T>::value
#ifdef HAS_ARRAYFIRE
		|| AfProxyConcept<T>
#endif
		;

	template <typename T>
	concept ComplexContainer = IsComplexContainer<T>::value
#ifdef HAS_ARRAYFIRE
		|| AfProxyConcept<T>
#endif
		;
}
