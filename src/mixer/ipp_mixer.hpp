#pragma once

#include "../common.hpp"
#include "mixer.hpp"
#include <ipp.h>

namespace ugsdr {
	class IppMixer : public Mixer<IppMixer> {
	protected:
		friend class Mixer<IppMixer>;

		static void Multiply(std::vector<std::complex<std::int8_t>>& src_dst, const std::vector<Ipp64fc>& c_exp) {
			thread_local std::vector<Ipp32fc> src_dst_fp32;
			CheckResize(src_dst_fp32, src_dst.size());
			ippsConvert_8s32f(reinterpret_cast<const Ipp8s*>(src_dst.data()), reinterpret_cast<Ipp32f*>(src_dst_fp32.data()), static_cast<int>(src_dst.size() * 2));

			thread_local std::vector<Ipp32fc> c_exp_fp32;
			CheckResize(c_exp_fp32, c_exp.size());
			ippsConvert_64f32f(reinterpret_cast<const Ipp64f*>(c_exp.data()), reinterpret_cast<Ipp32f*>(c_exp_fp32.data()), static_cast<int>(c_exp.size() * 2));

			ippsMul_32fc_I(reinterpret_cast<const Ipp32fc*>(c_exp_fp32.data()), reinterpret_cast<Ipp32fc*>(src_dst_fp32.data()), static_cast<int>(c_exp.size()));
			ippsConvert_32f8s_Sfs(reinterpret_cast<Ipp32f*>(src_dst_fp32.data()), reinterpret_cast<Ipp8s*>(src_dst.data()), static_cast<int>(src_dst.size() * 2), IppRoundMode::ippRndNear, 8);
		}
		static void Multiply(std::vector<std::complex<std::int32_t>>& src_dst, const std::vector<Ipp64fc>& c_exp) {
			thread_local std::vector<Ipp64fc> src_dst_fp64;
			CheckResize(src_dst_fp64, src_dst.size());
			ippsConvert_32s64f(reinterpret_cast<const Ipp32s*>(src_dst.data()), reinterpret_cast<Ipp64f*>(src_dst_fp64.data()), static_cast<int>(src_dst.size() * 2));

			ippsMul_64fc_I(c_exp.data(), src_dst_fp64.data(), static_cast<int>(c_exp.size()));
			ippsConvert_64f32s_Sfs(reinterpret_cast<Ipp64f*>(src_dst_fp64.data()), reinterpret_cast<Ipp32s*>(src_dst.data()), static_cast<int>(src_dst.size() * 2), IppRoundMode::ippRndNear, 8);
		}
		static void Multiply(std::vector<std::complex<float>>& src_dst, const std::vector<Ipp64fc>& c_exp) {
			thread_local std::vector<Ipp32fc> c_exp_fp32;
			CheckResize(c_exp_fp32, c_exp.size());
			ippsConvert_64f32f(reinterpret_cast<const Ipp64f*>(c_exp.data()), reinterpret_cast<Ipp32f*>(c_exp_fp32.data()), static_cast<int>(c_exp.size()) * 2);
			ippsMul_32fc_I(c_exp_fp32.data(), reinterpret_cast<Ipp32fc*>(src_dst.data()), static_cast<int>(c_exp.size()));
		}
		static void Multiply(std::vector<std::complex<float>>& src_dst, const std::vector<Ipp32fc>& c_exp) {
			ippsMul_32fc_I(c_exp.data(), reinterpret_cast<Ipp32fc*>(src_dst.data()), static_cast<int>(c_exp.size()));
		}
		static void Multiply(std::vector<std::complex<double>>& src_dst, const std::vector<Ipp64fc>& c_exp) {
			ippsMul_64fc_I(c_exp.data(), reinterpret_cast<Ipp64fc*>(src_dst.data()), static_cast<int>(c_exp.size()));
		}

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			if constexpr (std::is_integral_v<UnderlyingType>)
				scale = 127;

			thread_local std::vector<Ipp64fc> c_exp(src_dst.size());
			CheckResize(c_exp, src_dst.size());
			ippsTone_64fc(c_exp.data(), static_cast<int>(c_exp.size()), scale, frequency / sampling_freq, &phase, IppHintAlgorithm::ippAlgHintFast);

			Multiply(src_dst, c_exp);
		}

		static void Process(std::vector<std::complex<float>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			float scale = 1.0;

			thread_local std::vector<Ipp32fc> c_exp(src_dst.size());
			CheckResize(c_exp, src_dst.size());
			float phase_float = static_cast<float>(phase);
			ippsTone_32fc(c_exp.data(), static_cast<int>(c_exp.size()), scale, static_cast<Ipp32f>(frequency / sampling_freq), &phase_float, IppHintAlgorithm::ippAlgHintFast);
			
			Multiply(src_dst, c_exp);
		}

		static void Process(std::vector<std::complex<std::int16_t>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			Ipp16s scale = 127;

			//double pi_2 = 8 * std::atan(1.0);
			thread_local std::vector<Ipp16sc> c_exp(src_dst.size());
			CheckResize(c_exp, src_dst.size());
			float phase_fp32 = 0;
			ippsTone_16sc(c_exp.data(), static_cast<int>(c_exp.size()), scale, static_cast<Ipp32f>(frequency / sampling_freq), &phase_fp32, IppHintAlgorithm::ippAlgHintFast);

			ippsMul_16sc_ISfs(c_exp.data(), reinterpret_cast<Ipp16sc*>(src_dst.data()), static_cast<int>(src_dst.size()), 8);
		}

	public:
		IppMixer(double sampling_freq, double frequency, double phase) : Mixer<IppMixer>(sampling_freq, frequency, phase) {}

	};
}