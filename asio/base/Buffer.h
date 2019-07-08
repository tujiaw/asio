#ifndef ASIO_BASE_BUFFER_H_
#define ASIO_BASE_BUFFER_H_

#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <assert.h>

class Buffer
{
public:
	static const size_t kCheapPrepend = 0;
	static const size_t kInitialSize = 4096;

	Buffer()
		: buffer_(kCheapPrepend + kInitialSize)
		, readerIndex_(kCheapPrepend)
		, writerIndex_(kCheapPrepend)
	{
		assert(readableBytes() == 0);
		assert(writableBytes() == kInitialSize);
		assert(prependableBytes() == kCheapPrepend);
	}

	void swap(Buffer &rhs)
	{
		buffer_.swap(rhs.buffer_);
		std::swap(readerIndex_, rhs.readerIndex_);
		std::swap(writerIndex_, rhs.writerIndex_);
	}

	size_t readableBytes() const
	{
		return writerIndex_ - readerIndex_;
	}

	size_t writableBytes() const
	{
		return buffer_.size() - writerIndex_;
	}

	size_t prependableBytes() const
	{
		return readerIndex_;
	}

	const char *peek() const
	{
		return begin() + readerIndex_;
	}

	void retrieve(size_t len)
	{
		//assert(len <= readableBytes());
		if (len > readableBytes()) {
			int i = 0;
		}
		if (len < readableBytes())
		{
			readerIndex_ += len;
		} else
		{
			retrieveAll();
		}
	}

	void retrieveUntil(const char *end)
	{
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}

	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}

	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}

	void retrieveAll()
	{
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	std::string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}

	std::string retrieveAsString(size_t len)
	{
		assert(len <= readableBytes());
		std::string result(peek(), len);
		retrieve(len);
		return result;
	}

	void append(const std::string &str)
	{
		append(str.c_str(), str.size());
	}

	void append(const char* data, size_t len)
	{
		ensureWritableBytes(len);
		//std::copy(data, data + len, stdext::checked_array_iterator<char*>(beginWrite(), len));
        std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	void append(const void* data, size_t len)
	{
		append(static_cast<const char*>(data), len);
	}

	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}

	char* beginWrite()
	{
		return begin() + writerIndex_;
	}

	const char* beginWrite() const
	{
		return begin() + writerIndex_;
	}

	void hasWritten(size_t len)
	{
		writerIndex_ += len;
	}

	void appendInt32(int32_t x);
	void appendInt16(int16_t x);
	void appendInt8(int8_t x);
	int32_t readInt32();
	int16_t readInt16();
	int8_t readInt8();
	int32_t peekInt32() const;
	int16_t peekInt16() const;
	int8_t peekInt8() const;
	void prependInt32(int32_t x);
	void prependInt16(int16_t x);
	void prependInt8(int8_t x);

	bool read(void* data, size_t len)
	{
		if (data && readableBytes() >= len)
		{
			memcpy(data, peek(), len);
			retrieve(len);
			return true;
		}
		return false;
	}

	void prepend(const void* data, size_t len)
	{
		assert(len <= prependableBytes());
		readerIndex_ -= len;
		const char *d = static_cast<const char*>(data);
		//std::copy(d, d + len, stdext::checked_array_iterator<char*>(begin() + readerIndex_, len));
        std::copy(d, d + len, begin() + readerIndex_);
	}

private:
	char *begin()
	{
		return &*buffer_.begin();
	}

	const char* begin() const
	{
		return &*buffer_.begin();
	}

	void makeSpace(size_t len)
	{
		if (writableBytes() + prependableBytes() < len + kCheapPrepend)
		{
			buffer_.resize(writerIndex_ + len);
		} else
		{
			assert(kCheapPrepend < readerIndex_);
			size_t readable = readableBytes();
			//std::copy(begin() + readerIndex_, begin() + writerIndex_,
			//	stdext::checked_array_iterator<char*>(begin() + kCheapPrepend, writerIndex_ - readerIndex_));
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
			readerIndex_ = kCheapPrepend;
			writerIndex_ = readerIndex_ + readable;
			assert(readable == readableBytes());
		}
	}

	Buffer(const Buffer&);
	const Buffer& operator=(const Buffer&);

private:
	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;
};

#endif // ASIO_BASE_BUFFER_H_
