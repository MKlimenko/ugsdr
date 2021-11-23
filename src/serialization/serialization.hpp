#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>
#include <string>

namespace ugsdr {
	template <typename T>
	void Save(const std::string& path, const T& data_to_save) {
		std::ofstream data_stream(path, std::ios::binary);
		cereal::BinaryOutputArchive archive(data_stream);
		archive(data_to_save);
	}

	template <typename T>
	void Load(const std::string& path, T& data_to_load) {
		std::ifstream data_stream(path, std::ios::binary);
		cereal::BinaryInputArchive archive(data_stream);
		archive(data_to_load);
	}
	
	template <typename T>
	auto Load(const std::string& path) {
		std::ifstream data_stream(path, std::ios::binary);
		cereal::BinaryInputArchive archive(data_stream);
		T data_to_load;
		archive(data_to_load);
		return data_to_load;
	}
}
