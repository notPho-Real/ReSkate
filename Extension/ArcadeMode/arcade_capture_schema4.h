#pragma once
#include <cstdint>
#include <format>
#include <string>

namespace dingosdk::arcade_mode {
// Fixed version-four semantic payload. All native proof and completion values
// remain UNKNOWN until a separately qualified native primitive exists.
struct CaptureSchema4 {
    std::uint8_t m_version{1};
    std::uint8_t m_kind{};
    std::uint16_t m_required{255};
    std::uint16_t m_coverage{};
    std::uint16_t m_known{};
    std::uint8_t m_clock{};
    std::uint64_t m_source_ms{};
    std::uint64_t m_source_seq{};
    std::uint64_t m_poll{};
    std::uint64_t m_episode{};
    std::uint64_t m_departure{};
    std::uint64_t m_request{};
    std::uint64_t m_session{};
    bool m_from_air{};
    std::uint8_t m_query{};
    std::uint64_t m_deadline{};
    std::uint64_t m_age{};
    std::int8_t m_family{};
    std::uint8_t m_writer{};
    bool m_owns{};
    bool m_reserved{};
    bool m_neutral{};
    std::int16_t m_rx{};
    std::int16_t m_ry{};
    std::uint8_t m_native_proof{};
    std::uint64_t m_model{};
    std::uint8_t m_route{};
    std::uint64_t m_pub_ack{};
    std::uint8_t m_exit{};
    std::uint8_t m_completion{};
    std::uint64_t m_attempted{};
    std::uint64_t m_emitted{};
    std::uint64_t m_drained{};
    std::uint64_t m_dropped{};
    std::uint8_t q_initial{};
    std::uint8_t q_peak{};
    std::uint64_t q_attempted{};
    std::uint64_t q_emitted{};
    std::uint64_t q_drained{};
    std::uint64_t q_dropped{};
    std::uint64_t q_stall_ms{};
    bool q_budget_ok{};
    std::uint64_t log_epoch{};
    std::uint64_t log_part{};
    std::uint64_t log_record{};
    std::uint64_t log_bytes{};
    std::uint64_t log_total{};
    bool log_delivery{};
    std::uint64_t m_render_ack{};
    std::uint64_t m_render_frame{};
    std::uint64_t m_render_ms{};
    bool valid() const noexcept {
        return m_version==1 && m_required==255 && m_kind<=8 && m_known<=4095 && m_clock<=6 &&
            m_query<=2 && m_family>=-1 && m_family<=1 && m_writer<=6 && m_route<=3 && m_exit<=10 &&
            m_native_proof==0 && m_completion==0 && q_peak<=64 && q_initial<=8 &&
            m_drained<=m_emitted && m_emitted<=m_attempted && m_dropped<=m_attempted &&
            q_drained<=q_emitted && q_emitted<=q_attempted && q_dropped<=q_attempted;
    }
};
inline std::string capture_schema4_suffix(const CaptureSchema4& m) {
    return std::format(" m_version={} m_kind={} m_required={} m_coverage={} m_known={} m_clock={} m_source_ms={} m_source_seq={} m_poll={} m_episode={} m_departure={} m_request={} m_session={} m_from_air={} m_query={} m_deadline={} m_age={} m_family={} m_writer={} m_owns={} m_reserved={} m_neutral={} m_rx={} m_ry={} m_native_proof={} m_model={} m_route={} m_pub_ack={} m_exit={} m_completion={} m_attempted={} m_emitted={} m_drained={} m_dropped={} q_initial={} q_peak={} q_attempted={} q_emitted={} q_drained={} q_dropped={} q_stall_ms={} q_budget_ok={} log_epoch={} log_part={} log_record={} log_bytes={} log_total={} log_delivery={} m_render_ack={} m_render_frame={} m_render_ms={}.",
        static_cast<unsigned>(m.m_version),
        static_cast<unsigned>(m.m_kind),
        static_cast<unsigned>(m.m_required),
        static_cast<unsigned>(m.m_coverage),
        static_cast<unsigned>(m.m_known),
        static_cast<unsigned>(m.m_clock),
        m.m_source_ms,
        m.m_source_seq,
        m.m_poll,
        m.m_episode,
        m.m_departure,
        m.m_request,
        m.m_session,
        static_cast<unsigned>(m.m_from_air),
        static_cast<unsigned>(m.m_query),
        m.m_deadline,
        m.m_age,
        static_cast<int>(m.m_family),
        static_cast<unsigned>(m.m_writer),
        static_cast<unsigned>(m.m_owns),
        static_cast<unsigned>(m.m_reserved),
        static_cast<unsigned>(m.m_neutral),
        static_cast<int>(m.m_rx),
        static_cast<int>(m.m_ry),
        static_cast<unsigned>(m.m_native_proof),
        m.m_model,
        static_cast<unsigned>(m.m_route),
        m.m_pub_ack,
        static_cast<unsigned>(m.m_exit),
        static_cast<unsigned>(m.m_completion),
        m.m_attempted,
        m.m_emitted,
        m.m_drained,
        m.m_dropped,
        static_cast<unsigned>(m.q_initial),
        static_cast<unsigned>(m.q_peak),
        m.q_attempted,
        m.q_emitted,
        m.q_drained,
        m.q_dropped,
        m.q_stall_ms,
        static_cast<unsigned>(m.q_budget_ok),
        m.log_epoch,
        m.log_part,
        m.log_record,
        m.log_bytes,
        m.log_total,
        static_cast<unsigned>(m.log_delivery),
        m.m_render_ack,
        m.m_render_frame,
        m.m_render_ms
    );
}
}
