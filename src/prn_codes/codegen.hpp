#pragma once

#include "lfsr.hpp"

namespace ugsdr {
	template <typename CodegenImpl>
	class Codegen {
	protected:

	public:
		template <typename T>
		static void Get(T* prn) {
			CodegenImpl::Generate(prn);
		}
	};
}