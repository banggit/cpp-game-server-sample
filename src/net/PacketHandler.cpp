#include "net/PacketHandler.h"

#include "net/Session.h"
#include "net/PacketBuilder.h"
#include "logic/DbWorker.h"
#include "user/UserManager.h"
#include "user/User.h"
#include "common/Types.h"
#include "log/Logger.h"

#include <cstring>

namespace gs
{

bool PacketHandler::Dispatch(const PacketContext& in_ctx)
{
    if (in_ctx.PacketData.size() < PACKET_HEADER_SIZE)
    {
        LOG_WARN("packet too small from session " + std::to_string(in_ctx.SessionId));
        return false;
    }

    std::uint16_t opcode = 0;
    std::memcpy(&opcode, in_ctx.PacketData.data(), sizeof(opcode));

    switch (static_cast<PacketId>(opcode))
    {
        case PacketId::LOGIN_REQ:
            HandleLogin(in_ctx);
            break;

        case PacketId::ECHO_REQ:
            HandleEcho(in_ctx);
            break;

        case PacketId::HEARTBEAT_REQ:
            HandleHeartbeat(in_ctx);
            break;

        default:
            LOG_WARN("unknown opcode " + std::to_string(opcode)
                     + " from session " + std::to_string(in_ctx.SessionId));
            return false;
    }

    return true;
}

void PacketHandler::HandleLogin(const PacketContext& in_ctx)
{
    constexpr std::size_t REQUIRED_PAYLOAD = sizeof(AccountId);
    if (in_ctx.PacketData.size() < PACKET_HEADER_SIZE + REQUIRED_PAYLOAD)
    {
        LOG_WARN("LOGIN_REQ payload too small from session " + std::to_string(in_ctx.SessionId));
        return;
    }

    AccountId account_id = 0;
    std::memcpy(&account_id, in_ctx.PacketData.data() + PACKET_HEADER_SIZE, sizeof(AccountId));

    LOG_INFO("LOGIN_REQ session=" + std::to_string(in_ctx.SessionId)
             + " account_id=" + std::to_string(account_id));

    // 1. 메모리에 User 즉시 생성 + Session 바인딩.
    if (!m_user_manager)
    {
        LOG_ERROR("user manager not set");
        return;
    }

    auto user = m_user_manager->CreateUser(account_id, in_ctx.SessionId);
    if (!user)
    {
        // 중복 로그인 또는 invalid account_id.
        PacketBuilder response(PacketId::LOGIN_ACK);
        std::uint8_t result = 0;
        UserId zero_user_id = 0;
        response.Write(&result, 1);
        response.Write(zero_user_id);
        in_ctx.Session->SendPacket(response.Build());
        return;
    }

    in_ctx.Session->BindUser(user);

    // 2. LOGIN_ACK 즉시 응답 (메모리 처리만으로 응답 완료).
    PacketBuilder response(PacketId::LOGIN_ACK);
    std::uint8_t result = 1;
    response.Write(&result, 1);
    UserId user_id = user->GetUserId();
    response.Write(user_id);
    in_ctx.Session->SendPacket(response.Build());

    // 3. DB 에 로그인 기록 fire-and-forget.
    if (m_db_worker)
    {
        Job db_job(JobType::DB_LOG_LOGIN, in_ctx.SessionId, in_ctx.PacketData);
        m_db_worker->Push(db_job);
    }
}

void PacketHandler::HandleEcho(const PacketContext& in_ctx)
{
    LOG_DEBUG("ECHO_REQ from session " + std::to_string(in_ctx.SessionId));

    PacketBuilder response(PacketId::ECHO_ACK);
    if (in_ctx.PacketData.size() > PACKET_HEADER_SIZE)
    {
        const std::uint8_t* payload = in_ctx.PacketData.data() + PACKET_HEADER_SIZE;
        const std::size_t payload_size = in_ctx.PacketData.size() - PACKET_HEADER_SIZE;
        response.Write(payload, payload_size);
    }
    in_ctx.Session->SendPacket(response.Build());
}

void PacketHandler::HandleHeartbeat(const PacketContext& in_ctx)
{
    LOG_DEBUG("HEARTBEAT_REQ from session " + std::to_string(in_ctx.SessionId));

    PacketBuilder response(PacketId::HEARTBEAT_ACK);
    in_ctx.Session->SendPacket(response.Build());
}

} // namespace gs