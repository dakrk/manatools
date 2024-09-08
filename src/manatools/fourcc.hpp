#pragma once
#include <algorithm>
#include <string_view>

namespace manatools {
	class FourCC {
	public:
		constexpr FourCC() : data_() {};

		constexpr FourCC(const char* str) {
			size_t i;
			for (i = 0; i < 4; i++) {
				if (!str[i])
					break;
				data_[i] = str[i];
			}
			data_[i] = '\0';
		}

		constexpr FourCC(const std::string_view str) {
			size_t n = std::min(str.size(), 4lu);
			std::copy_n(str.begin(), n, data_);
			data_[n] = '\0';
		}

		constexpr bool operator==(const FourCC& rhs) const {
			return std::equal(data_, data_ + 4, rhs.data_);
		}

		constexpr char operator[](size_t i) const {
			return data_[i];
		}

		constexpr char& operator[](size_t i) {
			return data_[i];
		}

		constexpr const char* data() const {
			return data_;
		}

		constexpr char* data() {
			return data_;
		}

		constexpr std::string_view str() const {
			return {data_, 4};
		}

	private:
		char data_[5];
	};
} // namespace manatools
