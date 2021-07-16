#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

class VisualizerHelper final {
private:
	VisualizerHelper() {}

	std::mutex m;
	std::size_t counter = 0;

	template <typename T>
	void WriteFile(const std::wstring& filename, const std::vector<T>& vec) {
		std::ofstream file(filename, std::ios::binary);
		file.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(T));
	}

	template <typename T>
	auto GetSampleSize() {
		
	}
	
	template <typename T>
	auto FormCommandString(const std::wstring& filename, const std::vector<T>& vec) {
		auto command = LR"("%DS-5_Visualizer%\DS-5_Visualizer.exe" 1 )" + std::filesystem::absolute(filename).wstring() + L" ";
	}
public:
	static auto& GetHelper() {
		static VisualizerHelper helper;
		return helper;
	}

	template <typename T>
	void Add(const std::wstring& signal_name, const std::vector<T>& vec) {
		auto lock = std::unique_lock(m);

		auto filename = std::to_wstring(counter++) + L"_" + signal_name + L".bin";
		WriteFile(filename, vec);

		
	}
};