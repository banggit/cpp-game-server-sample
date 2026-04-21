#include "net/PacketBuffer.h"

#include "log/Logger.h"

namespace gs
{

PacketBuffer::PacketBuffer()
    : m_data()
{
}

void PacketBuffer::Clear()
{
    m_data.clear();
}

void PacketBuffer::Append(const std::uint8_t* in_data, std::size_t in_size)
{
    if (in_data && in_size > 0)
    {
        m_data.insert(m_data.end(), in_data, in_data + in_size);
    }
}

bool PacketBuffer::TryReadPacket(std::vector<std::uint8_t>& out_packet_data)
{
    if (m_data.size() < PACKET_HEADER_SIZE)
    {
        return false;
    }

    std::uint16_t packet_size = 0;
    if (!TryReadHeader(packet_size))
    {
        return false;
    }

    const std::size_t total_packet_size = PACKET_HEADER_SIZE + packet_size;
    if (m_data.size() < total_packet_size)
    {
        return false;
    }

    out_packet_data.assign(m_data.begin(), m_data.begin() + total_packet_size);
    RemoveProcessedBytes(total_packet_size);

    return true;
}

bool PacketBuffer::TryReadHeader(std::uint16_t& out_size)
{
    if (m_data.size() < PACKET_HEADER_SIZE)
    {
        return false;
    }

    std::uint16_t opcode = 0;
    std::uint16_t payload_size = 0;

    std::memcpy(&opcode, m_data.data(), sizeof(std::uint16_t));
    std::memcpy(&payload_size, m_data.data() + sizeof(std::uint16_t), sizeof(std::uint16_t));

    if (payload_size > MAX_PACKET_SIZE)
    {
        LOG_WARN("payload size exceeds max: " + std::to_string(payload_size));
        return false;
    }

    out_size = payload_size;
    return true;
}

void PacketBuffer::RemoveProcessedBytes(std::size_t in_bytes)
{
    if (in_bytes >= m_data.size())
    {
        m_data.clear();
    }
    else
    {
        m_data.erase(m_data.begin(), m_data.begin() + in_bytes);
    }
}

} // namespace gs