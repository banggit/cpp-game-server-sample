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

PacketReadResult PacketBuffer::TryReadPacket(std::vector<std::uint8_t>& out_packet_data)
{
    if (m_data.size() < PACKET_HEADER_SIZE)
    {
        return PacketReadResult::NEED_MORE;
    }

    std::uint16_t payload_size = 0;
    std::memcpy(&payload_size, m_data.data() + sizeof(std::uint16_t), sizeof(payload_size));

    if (payload_size > MAX_PACKET_SIZE)
    {
        // 복구 불가능한 스트림 오염 — 이후 바이트 경계를 신뢰할 수 없다.
        return PacketReadResult::INVALID;
    }

    const std::size_t total = PACKET_HEADER_SIZE + payload_size;
    if (m_data.size() < total)
    {
        return PacketReadResult::NEED_MORE;
    }

    out_packet_data.assign(m_data.begin(), m_data.begin() + total);
    RemoveProcessedBytes(total);
    return PacketReadResult::PACKET_READY;
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