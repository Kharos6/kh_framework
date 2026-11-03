#pragma once

// The shared head (rendering_integration.hpp): the graphics gate (render_on), reinit and the small helpers every
// part uses (steady_now_ms, mul_4x4, hr_str). Never include it on its own.
namespace RenderIntegration {
// PLAYER_ONLY (SQF COMMAND NOTES, rendering_integration.hpp): false = this machine has no graphics and nothing of
// the renderer may start.
// Tested wherever anything starts (the Draw3D and UI drivers, a new object or affector) and at the mission reset.
// framework.hpp's g_is_player: hasInterface, written once on the game thread in pre_start (main.cpp); every caller is
// on the game thread.
inline bool render_on() { return ::g_is_player; }

// SESSION_RESET - an object back to what a fresh process holds: destroyed and value-initialised in place (zero, then
// its members' own initialisers), as its static storage began; arrays element by element. Only on an object no other
// thread can be touching: the session teardown's park, or a thread's own memo (vs_memo's, reached through a
// thread_local pointer).
template <class T> inline void reinit(T& x) { x.~T(); ::new (static_cast<void*>(&x)) T(); }
template <class T, size_t N> inline void reinit(T (&x)[N]) { for (T& e : x) reinit(e); }

// EA_LOG_ONCE - a WARNING / INFO line (an expected degradation, a notice; never an error: those take
// report_error_once_safe) from ANY thread: logged to the RPT by sqf::diag_log on the game thread
// (MainThreadScheduler, as report_error_once_safe schedules its errors), never thrown into a script. Once per distinct
// text a session: g_log_once is cleared beside g_err_once by reset_session_state; past 256 distinct lines one
// capped notice, then nothing. Exit (EXIT_STATICS): a vector of strings, its destructor runs no engine code.
static std::mutex g_log_once_mx;
static std::vector<std::string> g_log_once;
static bool g_log_once_capped = false;   // Under g_log_once_mx.
inline void log_once_safe(const std::string& msg) {
    std::string line;   // Built only for a line that is logged.
    {
        std::lock_guard<std::mutex> g(g_log_once_mx);
        if (std::find(g_log_once.begin(), g_log_once.end(), msg) != g_log_once.end()) return;
        if (g_log_once.size() >= 256u) {
            if (g_log_once_capped) return;
            g_log_once_capped = true;
            line = "KH RenderIntegration: 256 distinct warnings this session; later ones are not logged (" +
                   msg + ")";
        } else {
            g_log_once.push_back(msg);
            line = msg;
        }
    }
    MainThreadScheduler::instance().schedule([line]() { sqf::diag_log(line); });
}

inline std::string hr_str(HRESULT hr) {
    char b[16];
    sprintf_s(b, "0x%08X", static_cast<unsigned>(hr));
    return std::string(b);
}

inline void mul_4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r][c] = a[r][0]*b[0][c] + a[r][1]*b[1][c] + a[r][2]*b[2][c] + a[r][3]*b[3][c];
}

inline uint64_t steady_now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
}