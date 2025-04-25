#ifndef LEAF_NET_BYTE_ORDER_H
#define LEAF_NET_BYTE_ORDER_H

#include <cstdint>
#include <cstddef>
#include <cstring> // for std::memcpy
#include <boost/endian/conversion.hpp>

// 网络字节序（大端）
inline uint64_t host_to_network64(uint64_t host64) { return boost::endian::native_to_big(host64); }
inline uint32_t host_to_network32(uint32_t host32) { return boost::endian::native_to_big(host32); }
inline uint16_t host_to_network16(uint16_t host16) { return boost::endian::native_to_big(host16); }

inline uint64_t network_to_host64(uint64_t net64) { return boost::endian::big_to_native(net64); }
inline uint32_t network_to_host32(uint32_t net32) { return boost::endian::big_to_native(net32); }
inline uint16_t network_to_host16(uint16_t net16) { return boost::endian::big_to_native(net16); }

static inline void be_write_uint16(uint8_t* ptr, uint16_t val)
{
    val = boost::endian::native_to_big(val);
    std::memcpy(ptr, &val, sizeof(val));
}

static inline void be_write_uint24(uint8_t* ptr, uint32_t val)
{
    val = boost::endian::native_to_big(val);
    ptr[0] = static_cast<uint8_t>((val >> 16) & 0xFF);
    ptr[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    ptr[2] = static_cast<uint8_t>(val & 0xFF);
}

static inline void be_write_uint32(uint8_t* ptr, uint32_t val)
{
    val = boost::endian::native_to_big(val);
    std::memcpy(ptr, &val, sizeof(val));
}

static inline void be_read_uint16(const uint8_t* ptr, uint16_t* val)
{
    std::memcpy(val, ptr, sizeof(uint16_t));
    *val = boost::endian::big_to_native(*val);
}

static inline void be_read_uint24(const uint8_t* ptr, uint32_t* val)
{
    *val = (ptr[0] << 16) | (ptr[1] << 8) | ptr[2];
}

static inline void be_read_uint32(const uint8_t* ptr, uint32_t* val)
{
    std::memcpy(val, ptr, sizeof(uint32_t));
    *val = boost::endian::big_to_native(*val);
}

static inline void le_write_uint32(uint8_t* ptr, uint32_t val)
{
    val = boost::endian::native_to_little(val);
    std::memcpy(ptr, &val, sizeof(val));
}

static inline void le_read_uint32(const uint8_t* ptr, uint32_t* val)
{
    std::memcpy(val, ptr, sizeof(uint32_t));
    *val = boost::endian::little_to_native(*val);
}

inline void set8(void* memory, std::size_t offset, uint8_t v)
{
    static_cast<uint8_t*>(memory)[offset] = v;
}

inline uint8_t get8(const void* memory, std::size_t offset)
{
    return static_cast<const uint8_t*>(memory)[offset];
}

inline void set_be16(void* memory, uint16_t v)
{
    *static_cast<uint16_t*>(memory) = boost::endian::native_to_big(v);
}

inline void set_be32(void* memory, uint32_t v)
{
    *static_cast<uint32_t*>(memory) = boost::endian::native_to_big(v);
}

inline void set_be64(void* memory, uint64_t v)
{
    *static_cast<uint64_t*>(memory) = boost::endian::native_to_big(v);
}

inline uint16_t get_be16(const void* memory)
{
    return boost::endian::big_to_native(*static_cast<const uint16_t*>(memory));
}

inline uint32_t get_be32(const void* memory)
{
    return boost::endian::big_to_native(*static_cast<const uint32_t*>(memory));
}

inline uint64_t get_be64(const void* memory)
{
    return boost::endian::big_to_native(*static_cast<const uint64_t*>(memory));
}

inline void set_le16(void* memory, uint16_t v)
{
    *static_cast<uint16_t*>(memory) = boost::endian::native_to_little(v);
}

inline void set_le32(void* memory, uint32_t v)
{
    *static_cast<uint32_t*>(memory) = boost::endian::native_to_little(v);
}

inline void set_le64(void* memory, uint64_t v)
{
    *static_cast<uint64_t*>(memory) = boost::endian::native_to_little(v);
}

inline uint16_t get_le16(const void* memory)
{
    return boost::endian::little_to_native(*static_cast<const uint16_t*>(memory));
}

inline uint32_t get_le32(const void* memory)
{
    return boost::endian::little_to_native(*static_cast<const uint32_t*>(memory));
}

inline uint64_t get_le64(const void* memory)
{
    return boost::endian::little_to_native(*static_cast<const uint64_t*>(memory));
}

#endif // LEAF_NET_BYTE_ORDER_H

