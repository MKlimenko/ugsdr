#pragma once

#ifdef HAS_CEREAL
#include <cereal/archives/binary.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/vector.hpp>
#else
#pragma message("Warning: cereal serialization library was not found, using stubs instead of serialization") 
#endif

#include <fstream>
#include <string>

namespace ugsdr {
	template <typename T>
	void Save(const std::string& path, const T& data_to_save) {
#ifdef HAS_CEREAL
		std::ofstream data_stream(path, std::ios::binary);
		cereal::BinaryOutputArchive archive(data_stream);
		archive(data_to_save);
#else
		static_cast<void>(path);
		static_cast<void>(data_to_save);
#endif
	}

	template <typename T>
	void Load(const std::string& path, T& data_to_load) {
		std::ifstream data_stream(path, std::ios::binary);
#ifdef HAS_CEREAL
		cereal::BinaryInputArchive archive(data_stream);
		archive(data_to_load);
#else
		static_cast<void>(data_to_load);
#endif
	}
	
	template <typename T>
	auto Load(const std::string& path) {
		T data_to_load{};
#ifdef HAS_CEREAL
		std::ifstream data_stream(path, std::ios::binary);
		cereal::BinaryInputArchive archive(data_stream);
		archive(data_to_load);
#endif
		return data_to_load;
	}
}
