#pragma once

#include "codegen.hpp"
#include "MemoryCodes.hpp"

#include <array>
#include <bitset>
#include <stdexcept>
#include <string>

namespace ugsdr {
	class GalileoE1c final : public Codegen<GalileoE1c>, MemoryCodes {
	private:
		static constexpr std::size_t memory_code_len = 4092;
		static const std::array<std::string, 50>& GetMemoryCodes();
	
	protected:
		friend class Codegen<GalileoE1c>;

		static auto NumberOfMilliseconds() {
			return 4;
		}

		static auto GetCodeLength() {
			return memory_code_len * 2;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			auto current_memory_code = GetMemoryCodes().at(sv_number);
			DecodeBoc(current_memory_code, prn, memory_code_len);
		}
	};
}
