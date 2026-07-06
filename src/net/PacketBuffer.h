#pragma once

#include "common/Types.h"

#include <cstdint>
#include <vector>
#include <cstring>

namespace gs
{


enum class PacketReadResult : std::uint8_t
{
    NEED_MORE,      // 아직 완성된 패킷 없음 (정상)
    PACKET_READY,   // 패킷 1개 추출 완료
    INVALID         // 프로토콜 위반 — 세션 절단 대상
};


class PacketBuffer
{
public:
    PacketBuffer();

    void Clear();
    void Append(const std::uint8_t* in_data, std::size_t in_size);

    PacketReadResult TryReadPacket(std::vector<std::uint8_t>& out_packet_data);

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