#pragma once

#include <cereal/archives/binary.hpp>
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
}
