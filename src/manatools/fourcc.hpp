#pragma once
#include <algorithm>
#include <cstring>
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
			size_t n = std::min<size_t>(str.size(), 4);
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
			return { data_, 4 };
		}

	private:
		char data_[5];
	};
} // namespace manatools

namespace std {
	template<> struct hash<manatools::FourCC> {
		/**
		 * Not allowed to just cast.
		 * Compiler should absolutely get the hint, though.
		 */
		size_t operator()(const manatools::FourCC& s) const {
			size_t h;
			memcpy(&h, s.data(), 4);
			return h;
		}
	};
} // namespace std
