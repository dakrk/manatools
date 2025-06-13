#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <system_error>

#include "io.hpp"
#include "utils.hpp"

#define POSIX_ERROR_CODE(err) std::error_code{err, std::generic_category()}

namespace manatools::io {

/* ======================== *
 *          DataIO          *
 * ======================== */

static DataIO::ErrorCategory DataIOErrorCategory;

const char* DataIO::ErrorCategory::name() const noexcept {
	return "DataIO";
}

std::string DataIO::ErrorCategory::message(int e) const {
	switch (static_cast<Error>(e)) {
		case Error::InvalidOperation: return "Invalid operation";
		case Error::FileNotOpen:      return "File not open";
		case Error::EndOfFile:        return "Unexpected end of data";
		case Error::ReadError:        return "Failed to read data";
		case Error::WriteError:       return "Failed to write data";
		case Error::FlushError:       return "Failed to flush data";
		case Error::CloseError:       return "Failed to close file";
		default:                      return "Unknown DataIO error";
	}
}

std::error_code DataIO::make_error_code(Error e) {
	return { static_cast<int>(e), DataIOErrorCategory };
}

/**
 * Errors in this are a bit flawed in the sense that I planned them so you
 * could pass any instance of these to anything, but the whole difference in
 * error handling you can choose makes that more difficult.
 * We mark this function as to not be inlined, because it avoids a lot of
 * bloat from creating exceptions in functions that use this, which is not
 * desirable as exceptions should not happen in the first place.
 */
MT_NOINLINE void DataIO::setError(std::error_code err) {
	if (!eofErrors_ && err == make_error_code(Error::EndOfFile)) {
		return;
	}

	error_ = err;

	if (error_ && exceptions_) {
		throw std::system_error(error_);
	}
}

/* ======================== *
 *          FileIO          *
 * ======================== */

bool FileIO::open(const char* path, const char* mode) {
	handle_ = fopen(path, mode);

	if (!handle_) {
		setError(POSIX_ERROR_CODE(errno));
		return false;
	}

	return true;
}

#ifdef _WIN32
bool FileIO::open(const wchar_t* path, const char* mode) {
	wchar_t modeW[10];
	size_t len = mbstowcs(modeW, mode, std::size(modeW));
	modeW[len] = '\0';

	handle_ = _wfopen(path, modeW);

	if (!handle_) {
		setError(POSIX_ERROR_CODE(errno));
		return false;
	}

	return true;
}
#endif

FileIO::~FileIO() {
	close();
}

// all these damn null handle checks
size_t FileIO::read(void* buf, size_t size, size_t count) {
	if (!handle_) {
		setError(Error::FileNotOpen);
		return 0;
	}

	size_t read = fread(buf, size, count, handle_);

	if (read != count) {
		if (feof(handle_)) {
			eof_ = true;
			setError(Error::EndOfFile);
		} else if (ferror(handle_)) {
			setError(Error::ReadError);
		}
	}

	return read;
}

size_t FileIO::write(const void* buf, size_t size, size_t count) {
	if (!handle_) {
		setError(Error::FileNotOpen);
		return 0;
	}

	size_t written = fwrite(buf, size, count, handle_);

	if (written != count)
		setError(Error::WriteError);

	return written;
}

bool FileIO::seek(long offset, Seek origin) {
	if (!handle_) {
		setError(Error::FileNotOpen);
		return false;
	}

	int forigin = 0;
	switch (origin) {
		case Seek::Set: { forigin = SEEK_SET; break; }
		case Seek::Cur: { forigin = SEEK_CUR; break; }
		case Seek::End: { forigin = SEEK_END; break; }
		default: assert(!"Invalid seek origin"); // Shouldn't get here
	}

	eof_ = false;

	if (fseek(handle_, offset, forigin)) {
		/**
		 * fseek setting errno is still an extension but cppreference mentions it so I can only
		 * presume it's widely implemented
		 */
		setError(POSIX_ERROR_CODE(errno));
		return false;
	}

	return true;
}

long FileIO::tell() {
	if (!handle_) {
		setError(Error::FileNotOpen);
		return false;
	}

	long pos = ftell(handle_);

	if (pos == -1L)
		setError(POSIX_ERROR_CODE(errno));

	return pos;
}

bool FileIO::flush() {
	if (!handle_) {
		setError(Error::FileNotOpen);
		return false;
	}

	if (fflush(handle_) == EOF) {
		setError(Error::FlushError);
		return false;
	}

	return true;
}

bool FileIO::close() {
	if (!handle_)
		return false;

	if (fclose(handle_) == EOF) {
		handle_ = nullptr;
		setError(Error::CloseError);
		return false;
	}

	handle_ = nullptr;
	return true;
}

/* ======================== *
 *         DynBufIO         *
 * ======================== */

size_t DynBufIO::read(void* buf, size_t size, size_t count) {
	size_t bytes = size * count;

	if (cur_ >= vec_.size() || !bytes) {
		eof_ = true;
		setError(Error::EndOfFile);
		return 0;
	}

	size_t toRead = std::min(bytes, vec_.size() - cur_);

	memcpy(buf, vec_.data() + cur_, toRead);
	cur_ += toRead;

	if (toRead != bytes) {
		eof_ = true;
		setError(Error::EndOfFile);
	}

	return toRead / size;
}

size_t DynBufIO::write(const void* buf, size_t size, size_t count) {
	size_t bytes = size * count;

	if (!bytes)
		return 0;

	size_t end = cur_ + bytes;

	if (end > vec_.size()) {
		vec_.resize(end);
	}

	memcpy(vec_.data() + cur_, buf, bytes);
	cur_ += bytes;

	return size;
}

bool DynBufIO::seek(long offset, Seek origin) {
	eof_ = false;

	switch (origin) {
		case Seek::Set: {
			cur_ = offset;
			break;
		}

		case Seek::Cur: {
			cur_ += offset;
			break;
		}

		case Seek::End: {
			cur_ = vec_.size() - offset;
			break;
		}

		default: {
			assert(!"Invalid seek origin");
		}
	}

	return true;
}

// size_t to long, great idea! (I hate this IO code)
long DynBufIO::tell() {
	return cur_;
}

/* ======================== *
 *          SpanIO          *
 * ======================== */

size_t SpanIO::read(void* buf, size_t size, size_t count) {
	size_t bytes = size * count;

	if (cur_ >= span_.size() || !bytes) {
		eof_ = true;
		setError(Error::EndOfFile);
		return 0;
	}

	size_t toRead = std::min(bytes, span_.size() - cur_);

	memcpy(buf, span_.data() + cur_, toRead);
	cur_ += toRead;

	if (toRead != bytes) {
		eof_ = true;
		setError(Error::EndOfFile);
	}

	return toRead / size;
}

size_t SpanIO::write(const void* buf, size_t size, size_t count) {
	size_t bytes = size * count;

	if (!bytes)
		return 0;

	memcpy(span_.data() + cur_, buf, bytes);
	cur_ += bytes;

	return size;
}

bool SpanIO::seek(long offset, Seek origin) {
	eof_ = false;

	switch (origin) {
		case Seek::Set: {
			cur_ = offset;
			break;
		}

		case Seek::Cur: {
			cur_ += offset;
			break;
		}

		case Seek::End: {
			cur_ = span_.size() - offset;
			break;
		}

		default: {
			assert(!"Invalid seek origin");
		}
	}

	return true;
}

long SpanIO::tell() {
	return cur_;
}

} // namespace manatools::io
