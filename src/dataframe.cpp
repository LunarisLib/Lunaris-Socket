#include <Lunaris/socket.h>

#include <cstring>
#include <stdexcept>

namespace Lunaris {
namespace Socket {

    static_assert(sizeof(uint64_t) == 8, "How is your uint64_t not 8 bytes?");
    static_assert(sizeof(unsigned char) == 1, "Invalid unsigned char size. It must be 1 byte");

    union __caster {
        uint64_t len;
        unsigned char eq[8];
    };

    DataFrame& DataFrame::operator<<(std::string v)
    {
        __caster cast;
        cast.len = v.size();
        _push(cast.eq, 8);
        _push(v.c_str(), v.length());
        return *this;
    }

    DataFrame& DataFrame::operator<<(uint16_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(uint32_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(uint64_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(int16_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(int32_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(int64_t v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(float v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }

    DataFrame& DataFrame::operator<<(double v)
    {
        _push((char*)&v, sizeof(v));
        return *this;
    }


    DataFrame& DataFrame::operator>>(std::string& v)
    {
        __caster de_cast;
        _read(&de_cast, 8);
        if (de_cast.len > m_mem.size())
            throw std::runtime_error("Incorrect block size read, data is corrupted?");
        
        v = std::string(de_cast.len, '\0');
        _read(v.data(), de_cast.len);
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(uint16_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(uint32_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(uint64_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(int16_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(int32_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(int64_t& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(float& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    DataFrame& DataFrame::operator>>(double& v)
    {
        _read(&v, sizeof(v));
        return *this;
    }
    
    const unsigned char* DataFrame::data() const
    {
        return m_mem.data();
    }

    size_t DataFrame::size() const
    {
        return m_mem.size();
    }

    void DataFrame::set(const unsigned char* data, const size_t len)
    {
        m_mem.assign(data, data + len);
        m_read_off = 0;
    }

    void DataFrame::_push(const void* data, const size_t len)
    {
        std::vector<unsigned char> vec((unsigned char*)data, (unsigned char*)data + len);
        m_mem.insert(m_mem.end(), std::move_iterator(vec.begin()), std::move_iterator(vec.end()));
        m_read_off = 0;
    }

    void DataFrame::_read(void* data, const size_t len)
    {
        if (len + m_read_off > m_mem.size())
            throw std::runtime_error("Trying to read more than there was available!");
        
        const unsigned char* ref = m_mem.data();
        memcpy(data, ref + m_read_off, len);
        m_read_off += len;
    }

}
}