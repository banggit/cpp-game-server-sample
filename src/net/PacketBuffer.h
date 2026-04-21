#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <cstring>

namespace gs
{

class PacketBuffer
{
public:
    PacketBuffer();

    void Clear();
    void Append(const std::uint8_t* in_data, std::size_t in_size);

    bool TryReadPacket(std::vector<std::uint8_t>& out_packet_data);

    std::size_t GetBufferSize() const
    {
        return m_data.size();
    }

private:
    bool TryReadHeader(std::uint16_t& out_size);
    void RemoveProcessedBytes(std::size_t in_bytes);

    std::vector<std::uint8_t> m_data;
};

} // namespace gs