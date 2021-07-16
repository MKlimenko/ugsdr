#pragma once

#include "lfsr.hpp"

namespace ugsdr {
	template <typename CodegenImpl>
	class Codegen {
	protected:
		template <typename T>
		static const auto& GetVector(std::size_t sv_number) {
			static std::vector<T> code(CodegenImpl::GetCodeLength());
			Get(code.data(), sv_number);
			return code;
		}
		
	public:
		template <typename T>
		static void Get(T* prn, std::size_t sv_number) {
			CodegenImpl::Generate(prn, sv_number);
		}

		template <typename T>
		static const auto& Get(std::size_t sv_number) {
			return GetVector<T>(sv_number);
		}

		template <typename T>
		static void Get(std::vector<T>& dst, std::size_t sv_number) {
			dst = GetVector<T>(sv_number);
		}
	};
}