#include "net/PacketBuilder.h"

#include <cstring>

namespace gs
{

PacketBuilder::PacketBuilder(PacketId in_opcode)
    : m_buffer()
{
    std::uint16_t opcode = static_cast<std::uint16_t>(in_opcode);
    const std::uint8_t* opcode_ptr = reinterpret_cast<const std::uint8_t*>(&opcode);
    m_buffer.insert(m_buffer.end(), opcode_ptr, opcode_ptr + sizeof(std::uint16_t));
    
    std::uint16_t size_placeholder = 0;
    const std::uint8_t* size_ptr = reinterpret_cast<const std::uint8_t*>(&size_placeholder);
    m_buffer.insert(m_buffer.end(), size_ptr, size_ptr + sizeof(std::uint16_t));
}

PacketBuilder& PacketBuilder::Write(const std::uint8_t* in_data, std::size_t in_size)
{
    if (in_data && in_size > 0)
    {
        m_buffer.insert(m_buffer.end(), in_data, in_data + in_size);
    }
    return *this;
}

const std::vector<std::uint8_t>& PacketBuilder::Build()
{
    std::uint16_t payload_size = static_cast<std::uint16_t>(m_buffer.size() - PACKET_HEADER_SIZE);
    std::memcpy(m_buffer.data() + sizeof(std::uint16_t), &payload_size, sizeof(std::uint16_t));
    return m_buffer;
}

} // namespace gs