#ifndef LEAF_NET_BYTE_ORDER_H
#define LEAF_NET_BYTE_ORDER_H

#include <cstdint>

#ifdef __MACH__
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#else
#include <endian.h>
#endif

inline uint64_t host_to_network64(uint64_t host64) { return htobe64(host64); }

inline uint32_t host_to_network32(uint32_t host32) { return htobe32(host32); }

inline uint16_t host_to_network16(uint16_t host16) { return htobe16(host16); }

inline uint64_t network_to_host64(uint64_t net64) { return be64toh(net64); }

inline uint32_t network_to_host32(uint32_t net32) { return be32toh(net32); }

inline uint16_t network_to_host16(uint16_t net16) { return be16toh(net16); }

static inline void be_write_uint16(uint8_t* ptr, uint16_t val)
{
    ptr[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[1] = static_cast<uint8_t>(val & 0xFF);
}

static inline void be_write_uint24(uint8_t* ptr, uint32_t val)
{
    ptr[0] = static_cast<uint8_t>((val >> 16) & 0xFF);
    ptr[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[2] = static_cast<uint8_t>(val & 0xFF);
}

static inline void be_write_uint32(uint8_t* ptr, uint32_t val)
{
    ptr[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    ptr[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    ptr[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[3] = static_cast<uint8_t>(val & 0xFF);
}

static inline void be_read_uint16(const uint8_t* ptr, uint16_t* val) { *val = (ptr[0] << 8) | ptr[1]; }

static inline void be_read_uint24(const uint8_t* ptr, uint32_t* val) { *val = (ptr[0] << 16) | (ptr[1] << 8) | ptr[2]; }

static inline void be_read_uint32(const uint8_t* ptr, uint32_t* val)
{
    *val = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

static inline void le_write_uint32(uint8_t* ptr, uint32_t val)
{
    ptr[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    ptr[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    ptr[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[0] = static_cast<uint8_t>(val & 0xFF);
}

static inline void le_read_uint32(const uint8_t* ptr, uint32_t* val)
{
    *val = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

inline void set8(void* memory, std::size_t offset, uint8_t v) { static_cast<uint8_t*>(memory)[offset] = v; }

inline uint8_t get8(const void* memory, std::size_t offset) { return static_cast<const uint8_t*>(memory)[offset]; }

inline void set_be16(void* memory, uint16_t v) { *static_cast<uint16_t*>(memory) = htobe16(v); }

inline void set_be32(void* memory, uint32_t v) { *static_cast<uint32_t*>(memory) = htobe32(v); }

inline void set_be64(void* memory, uint64_t v) { *static_cast<uint64_t*>(memory) = htobe64(v); }

inline uint16_t get_be16(const void* memory) { return be16toh(*static_cast<const uint16_t*>(memory)); }

inline uint32_t get_be32(const void* memory) { return be32toh(*static_cast<const uint32_t*>(memory)); }

inline uint64_t get_be64(const void* memory) { return be64toh(*static_cast<const uint64_t*>(memory)); }

inline void set_le16(void* memory, uint16_t v) { *static_cast<uint16_t*>(memory) = htole16(v); }

inline void set_le32(void* memory, uint32_t v) { *static_cast<uint32_t*>(memory) = htole32(v); }

inline void set_le64(void* memory, uint64_t v) { *static_cast<uint64_t*>(memory) = htole64(v); }

inline uint16_t get_le16(const void* memory) { return le16toh(*static_cast<const uint16_t*>(memory)); }

inline uint32_t get_le32(const void* memory) { return le32toh(*static_cast<const uint32_t*>(memory)); }

inline uint64_t get_le64(const void* memory) { return le64toh(*static_cast<const uint64_t*>(memory)); }

#endif
