#ifndef LEAF_NET_BUFFER_H
#define LEAF_NET_BUFFER_H

#include <vector>
#include <string>
#include <cstring>
#include "byte_order.h"

namespace leaf
{
class write_buffer
{
   public:
    write_buffer() = default;
    write_buffer(const write_buffer&) = delete;
    write_buffer& operator=(const write_buffer&) = delete;

   public:
    void swap(write_buffer& rhs) { buffer_.swap(rhs.buffer_); }
    std::size_t size() const { return buffer_.size(); }
    const char* data() const { return buffer_.data(); }

    std::size_t copy_to(void* data, std::size_t size)
    {
        if (data == nullptr)
        {
            return 0;
        }
        auto min_size = std::min(size, buffer_.size());
        ::memcpy(data, buffer_.data(), min_size);
        return min_size;
    }

    void copy_to(std::vector<uint8_t>* bytes) { bytes->insert(bytes->end(), buffer_.begin(), buffer_.end()); }

    void write_bytes(const char* data, std::size_t len)
    {
        //
        buffer_.insert(buffer_.end(), data, data + len);
    }

    void write_bytes(const uint8_t* data, std::size_t len) { buffer_.insert(buffer_.end(), data, data + len); }

    void write_bytes(const std::vector<uint8_t>& buff) { buffer_.insert(buffer_.end(), buff.begin(), buff.end()); }

    void write_uint64(uint64_t x)
    {
        uint64_t val = host_to_network64(x);
        write_bytes(reinterpret_cast<const char*>(&val), sizeof val);
    }

    void write_uint32(uint32_t x)
    {
        uint32_t val = host_to_network32(x);
        write_bytes(reinterpret_cast<const char*>(&val), sizeof val);
    }

    void write_uint16(uint16_t x)
    {
        uint16_t val = host_to_network16(x);
        write_bytes(reinterpret_cast<const char*>(&val), sizeof val);
    }

    void write_uint8(uint8_t x) { write_bytes(reinterpret_cast<const char*>(&x), sizeof x); }

   private:
    std::vector<char> buffer_;
};

class read_buffer
{
   public:
    read_buffer(const char* data, std::size_t size) : data_(data), size_(size) {}
    read_buffer(const void* data, std::size_t size) : data_(static_cast<const char*>(data)), size_(size) {}
    read_buffer(const read_buffer&) = delete;
    read_buffer& operator=(const read_buffer&) = delete;

   public:
    std::size_t size() const { return size_ - start_; }
    const char* peek() const { return data_ + start_; }
    const char* data() const { return data_ + start_; }

    bool read_uint64(uint64_t* val)
    {
        if (!peek_uint64(val))
        {
            return false;
        }
        consume(sizeof *val);
        return true;
    }

    bool read_uint32(uint32_t* val)
    {
        if (!peek_uint32(val))
        {
            return false;
        }
        consume(sizeof *val);
        return true;
    }

    bool read_uint16(uint16_t* val)
    {
        if (!peek_uint16(val))
        {
            return false;
        }
        consume(sizeof *val);
        return true;
    }

    bool read_uint8(uint8_t* val)
    {
        if (!peek_uint8(val))
        {
            return false;
        }
        consume(sizeof *val);
        return true;
    }

    bool peek_uint64(uint64_t* val) const
    {
        if (val == nullptr)
        {
            return false;
        }
        uint64_t v = 0;
        if (!peek_bytes(reinterpret_cast<char*>(&v), sizeof v))
        {
            return false;
        }
        *val = network_to_host64(v);
        return true;
    }

    bool peek_uint32(uint32_t* val) const
    {
        if (val == nullptr)
        {
            return false;
        }
        uint32_t v = 0;
        if (!peek_bytes(reinterpret_cast<char*>(&v), sizeof v))
        {
            return false;
        }
        *val = network_to_host32(v);
        return true;
    }

    bool peek_uint16(uint16_t* val) const
    {
        if (val == nullptr)
        {
            return false;
        }
        uint16_t v = 0;
        if (!peek_bytes(reinterpret_cast<char*>(&v), sizeof v))
        {
            return false;
        }
        *val = network_to_host16(v);
        return true;
    }

    bool peek_uint8(uint8_t* val) const
    {
        if (val == nullptr)
        {
            return false;
        }
        return peek_bytes(reinterpret_cast<char*>(val), sizeof *val);
    }

    bool read_bytes(void* val, std::size_t len)
    {
        if (val == nullptr)
        {
            return false;
        }
        if (start_ + len > size_)
        {
            return false;
        }
        ::memcpy(val, peek(), len);
        consume(len);
        return true;
    }
    bool read_string(std::string* str, std::size_t len)
    {
        if (start_ + len > size_)
        {
            return false;
        }
        str->assign(peek(), len);
        consume(len);
        return true;
    }

    bool peek_bytes(void* val, std::size_t len) const
    {
        if (start_ + len > size_)
        {
            return false;
        }
        ::memcpy(val, peek(), len);
        return true;
    }

    void consume(std::size_t size) { start_ += std::min(size, this->size()); }

   private:
    std::size_t start_ = 0;
    const char* data_ = nullptr;
    std::size_t size_ = 0;
};
}    // namespace leaf
#endif
