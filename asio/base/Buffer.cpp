#include "Buffer.h"
#include <boost/asio.hpp>

void Buffer::appendInt32(int32_t x)
{
	append(&x, sizeof(x));
}

void Buffer::appendInt16(int16_t x)
{
	append(&x, sizeof(x));
}

void Buffer::appendInt8(int8_t x)
{
	append(&x, sizeof(x));
}

int32_t Buffer::readInt32()
{
	int32_t result = peekInt32();
	retrieveInt32();
	return result;
}

int16_t Buffer::readInt16()
{
	int16_t result = peekInt16();
	retrieveInt16();
	return result;
}

int8_t Buffer::readInt8()
{
	int8_t result = peekInt8();
	retrieveInt8();
	return result;
}

int32_t Buffer::peekInt32() const
{
	assert(readableBytes() >= sizeof(int32_t));
	int32_t be32 = 0;
	::memcpy(&be32, peek(), sizeof(be32));
	return be32;
}

int16_t Buffer::peekInt16() const
{
	assert(readableBytes() >= sizeof(int16_t));
	int16_t be16 = 0;
	::memcpy(&be16, peek(), sizeof(be16));
	return be16;
}

int8_t Buffer::peekInt8() const
{
	assert(readableBytes() >= sizeof(int8_t));
	int8_t x = *peek();
	return x;
}

void Buffer::prependInt32(int32_t x)
{
	prepend(&x, sizeof(x));
}

void Buffer::prependInt16(int16_t x)
{
	prepend(&x, sizeof(x));
}

void Buffer::prependInt8(int8_t x)
{
	prepend(&x, sizeof(x));
}
