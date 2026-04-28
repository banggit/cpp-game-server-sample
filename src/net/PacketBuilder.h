#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <cstring>

namespace gs
{

class PacketBuilder
{
public:
    PacketBuilder(PacketId in_opcode);

    PacketBuilder& Write(const std::uint8_t* in_data, std::size_t in_size);
    
    template<typename T>
    PacketBuilder& Write(const T& in_value)
    {
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
        const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(&in_value);
        return Write(ptr, sizeof(T));
    }

    const std::vector<std::uint8_t>& Build();

private:
    std::vector<std::uint8_t> m_buffer;
};

} // namespace gs