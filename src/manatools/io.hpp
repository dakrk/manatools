#pragma once
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

#include "endian.hpp"
#include "filesystem.hpp"
#include "types.hpp"

// messy and not very good but whatever. works well enough.

namespace manatools::io {
	class DataIO {
	public:
		DataIO(const DataIO&) = delete;
		DataIO& operator=(const DataIO&) = delete;
		virtual ~DataIO() = default;

		enum class Seek {
			Set,
			Cur,
			End
		};

		virtual size_t read(void* buf, size_t size, size_t count) = 0;
		virtual size_t write(const void* buf, size_t size, size_t count) = 0;
		virtual bool seek(long offset, Seek origin) = 0;
		virtual long tell() = 0;

		bool jump(long offset)                             { return seek(offset, Seek::Set); }
		bool forward(long offset)                          { return seek(offset, Seek::Cur); }
		bool end(long offset = 0)                          { return seek(offset, Seek::End); }

		bool readU8(u8* out)                               { return read(out, sizeof(*out), 1) == 1; }
		bool readS8(s8* out)                               { return read(out, sizeof(*out), 1) == 1; }
		bool readU16LE(u16* out);
		bool readU16BE(u16* out);
		bool readU32LE(u32* out);
		bool readU32BE(u32* out);
		bool readBool(bool* out);

		bool writeU8(u8 in)                                { return write(&in, sizeof(in), 1) == 1; }
		bool writeS8(s8 in)                                { return write(&in, sizeof(in), 1) == 1; }
		bool writeU16LE(u16 in);
		bool writeU16BE(u16 in);
		bool writeU32LE(u32 in);
		bool writeU32BE(u32 in);
		bool writeBool(bool in);
		bool writeStr(const std::string_view in)           { return write(in.data(), sizeof(char), in.size()) == in.size(); }

		template <typename T>
		bool readT(T* out)                                 { return read(out, sizeof(*out), 1) == 1; }

		template <typename T, size_t N>
		bool readArrT(T (&buf)[N])                         { return read(buf, sizeof(T), N) == N; }

		template <typename T>
		bool readVec(std::vector<T>& in)                   { return read(in.data(), sizeof(T), in.size()) == in.size(); }

		template <typename T, size_t Extent>
		bool readSpan(std::span<T, Extent> in)             { return read(in.data(), sizeof(T), in.size()) == in.size(); }

		template <typename T>
		bool writeT(T in)                                  { return write(&in, sizeof(in), 1) == 1; }

		template <typename T, size_t N>
		bool writeArrT(const T (&buf)[N])                  { return write(buf, sizeof(T), N) == N; }

		template <typename T>
		bool writeVec(const std::vector<T>& in)            { return write(in.data(), sizeof(T), in.size()) == in.size(); }

		template <typename T, size_t Extent>
		bool writeSpan(const std::span<T, Extent> in)      { return write(in.data(), sizeof(T), in.size()) == in.size(); }

		/* ======================== *
		 *      Error handling      *
		 * ======================== */

		bool good() const                                  { return !error_ && !eof_; }
		inline bool eof() const                            { return eof_; }
		std::error_code error() const                      { return error_; }

		explicit operator bool() const                     { return !error(); }

		virtual void clear() {
			eof_ = false;
			error_.clear();
		}

		bool exceptions() const {
			return exceptions_;
		}
		
		bool exceptions(bool state) {
			bool old = exceptions_;
			exceptions_ = state;
			setError(error_);
			return old;
		}

		bool eofErrors() const {
			return eofErrors_;
		}

		bool eofErrors(bool state) {
			bool old = eofErrors_;
			eofErrors_ = state;
			setError(error_);
			return old;
		}

		// yeah... some of these are file specific and should be in FileIO instead but whatever
		enum class Error {
			InvalidOperation = 1,
			FileNotOpen,
			EndOfFile,
			ReadError,
			WriteError,
			FlushError,
			CloseError
		};

		class ErrorCategoryImpl final : public std::error_category {
			virtual const char* name() const noexcept override;
			virtual std::string message(int e) const override;
		};
		
		const std::error_category& ErrorCategory();
		std::error_code make_error_code(Error e);

	protected:
		DataIO(bool exceptions, bool eofErrors) :
			exceptions_(exceptions),
			eofErrors_(eofErrors) {}

		/**
		 * errors in this are a bit flawed in the sense that I planned them so
		 * you could pass any instance of these to anything, but the whole
		 * difference in error handling you can choose makes that more difficult
		 */
		void setError(std::error_code err) {
			if (!eofErrors_ && err == make_error_code(Error::EndOfFile))
				return;

			error_ = err;

			if (error_ && exceptions_)
				throw std::system_error(error_);
		}

		void setError(Error err) {
			setError(make_error_code(err));
		}

		// ugh... not so satisfied with how EOF is handled
		bool eof_ = false;
		std::error_code error_;

		bool exceptions_ = true;
		bool eofErrors_ = false;
	};

	#define DEFINE_READ_FUNC(type, endian, name) \
		inline bool DataIO::read##name(type* out) { \
			bool ret = read(out, sizeof(*out), 1) == 1; \
			if (ret) \
				*out = endian(*out); \
			return ret; \
		}

	#define DEFINE_WRITE_FUNC(type, endian, name) \
		inline bool DataIO::write##name(type in) { \
			in = endian(in); \
			return write(&in, sizeof(in), 1) == 1; \
		}

	DEFINE_READ_FUNC(u16, LE, U16LE)
	DEFINE_READ_FUNC(u16, BE, U16BE)
	DEFINE_READ_FUNC(u32, LE, U32LE)
	DEFINE_READ_FUNC(u32, BE, U32BE)

	inline bool DataIO::readBool(bool* out) {
		u8 b;
		bool ret = readU8(&b);
		if (ret)
			*out = b;
		return ret;
	}

	DEFINE_WRITE_FUNC(u16, LE, U16LE)
	DEFINE_WRITE_FUNC(u16, BE, U16BE)
	DEFINE_WRITE_FUNC(u32, LE, U32LE)
	DEFINE_WRITE_FUNC(u32, BE, U32BE)

	inline bool DataIO::writeBool(bool in) {
		u8 b = in;
		return write(&b, sizeof(b), 1) == 1;
	}

	#undef DEFINE_READ_FUNC
	#undef DEFINE_WRITE_FUNC

	class FileIO : public DataIO {
	public:
		FileIO(const char* path,         const char* mode, bool exceptions = true, bool eofErrors = true);
		FileIO(const std::string& path,  const char* mode, bool exceptions = true, bool eofErrors = true) :
			FileIO(path.c_str(), mode, exceptions, eofErrors) {}
		FileIO(const fs::path& path,     const char* mode, bool exceptions = true, bool eofErrors = true) :
			FileIO(path.string().c_str(), mode, exceptions, eofErrors) {}

		// all this wide char and string stuff. i hate windows
		#ifdef _WIN32
		FileIO(const wchar_t* path,      const wchar_t* mode, bool exceptions = true, bool eofErrors = true);
		FileIO(const std::wstring& path, const wchar_t* mode, bool exceptions = true, bool eofErrors = true) :
			FileIO(path.c_str(), mode, exceptions, eofErrors) {}
		#endif

		~FileIO() override;

		size_t read(void* buf, size_t size, size_t count) override;
		size_t write(const void* buf, size_t size, size_t count) override;
		bool seek(long offset, Seek origin) override;
		long tell() override;

		bool flush();
		bool close();

		bool isOpen() const { return handle_; }

	private:
		FILE* handle_ = nullptr;
	};

	class DynBufIO : public DataIO {
	public:
		typedef std::vector<u8> VecType;

		DynBufIO(VecType& vec, bool exceptions = true, bool eofErrors = true) :
			DataIO(exceptions, eofErrors), vec_(vec), cur_(0) {}

		size_t read(void* buf, size_t size, size_t count) override;
		size_t write(const void* buf, size_t size, size_t count) override;
		bool seek(long offset, Seek origin) override;
		long tell() override;

		VecType& vec()                         { return vec_; }
		
		VecType::size_type size() const        { return vec_.size(); }
		VecType::size_type capacity() const    { return vec_.capacity(); }
		void resize(VecType::size_type count)  { return vec_.resize(count); }
		void reserve(VecType::size_type count) { return vec_.reserve(count); }

		void resize(VecType::size_type cnt, const VecType::value_type& val) {
			return vec_.resize(cnt, val);
		}

	private:
		VecType& vec_;
		size_t cur_;
	};
} // namespace manatools::io

// yep
namespace std {
	template<>
	struct is_error_code_enum<manatools::io::DataIO::Error> : true_type {};
} // namespace std
