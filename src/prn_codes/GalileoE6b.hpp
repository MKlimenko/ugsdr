#pragma once

#include "codegen.hpp"
#include "MemoryCodes.hpp"

#include <array>
#include <bitset>
#include <stdexcept>
#include <string>

namespace ugsdr {
	class GalileoE6b final : public Codegen<GalileoE6b>, MemoryCodes {
	private:
		static constexpr std::size_t memory_code_len = 5115;
		static const std::array<std::string, 50>& GetMemoryCodes();
	
	protected:
		friend class Codegen<GalileoE6b>;

		static auto NumberOfMilliseconds() {
			return 1;
		}
		
		static auto GetCodeLength() {
			return memory_code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			auto current_memory_code = GetMemoryCodes().at(sv_number);
			Decode(current_memory_code, prn, memory_code_len);
		}
	};
}
