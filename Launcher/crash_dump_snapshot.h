#pragma once
#include <Windows.h>
#include <DbgHelp.h>
#include <processsnapshot.h>
#include <bit>

namespace dingosdk::crash_report {
// Stuck-load captures inspect a running process. A VA clone keeps its stacks
// and referenced memory stable while DbgHelp writes the dump. Resolve the API
// optionally so hosts without PSS retain the existing live capture path.
class DumpSnapshot {
public:
    DumpSnapshot(HANDLE process, bool requested) noexcept {
        if (!requested) return;
        const auto kernel = GetModuleHandleW(L"kernel32.dll");
        if (!kernel) { error_ = GetLastError(); return; }
        const auto capture = std::bit_cast<decltype(&PssCaptureSnapshot)>(GetProcAddress(kernel, "PssCaptureSnapshot"));
        free_ = std::bit_cast<decltype(&PssFreeSnapshot)>(GetProcAddress(kernel, "PssFreeSnapshot"));
        if (!capture || !free_) { error_ = ERROR_CALL_NOT_IMPLEMENTED; return; }
        // The inherited parent handle remains read-only. This temporary handle
        // grants only the additional rights needed to clone that same process.
        const auto target = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
            PROCESS_CREATE_PROCESS | PROCESS_DUP_HANDLE, FALSE, GetProcessId(process));
        if (!target) { error_ = GetLastError(); return; }
        constexpr auto flags = static_cast<PSS_CAPTURE_FLAGS>(PSS_CAPTURE_VA_CLONE |
            PSS_CAPTURE_THREADS | PSS_CAPTURE_THREAD_CONTEXT | PSS_CAPTURE_VA_SPACE |
            PSS_CAPTURE_VA_SPACE_SECTION_INFORMATION);
        error_ = capture(target, flags, CONTEXT_ALL, &snapshot_);
        CloseHandle(target);
    }
    ~DumpSnapshot() { if (snapshot_ && free_) free_(GetCurrentProcess(), snapshot_); }
    DumpSnapshot(const DumpSnapshot&) = delete;
    DumpSnapshot& operator=(const DumpSnapshot&) = delete;
    HANDLE handle(HANDLE fallback) const noexcept { return snapshot_ ? reinterpret_cast<HANDLE>(snapshot_) : fallback; }
    bool captured() const noexcept { return snapshot_ != nullptr; }
    DWORD error() const noexcept { return error_; }
    static BOOL CALLBACK callback(void*, MINIDUMP_CALLBACK_INPUT* input, MINIDUMP_CALLBACK_OUTPUT* output) noexcept {
        switch (input->CallbackType) {
        case IsProcessSnapshotCallback: output->Status = S_FALSE; return TRUE;
        case IoStartCallback:
        case VmStartCallback: output->Status = S_OK; return TRUE;
        // Do not silently omit unreadable memory or replace normal dump I/O.
        case ReadMemoryFailureCallback: output->Status = input->ReadMemoryFailure.FailureStatus; return FALSE;
        case MemoryCallback:
        case RemoveMemoryCallback:
        case WriteKernelMinidumpCallback: return FALSE;
        default: return TRUE;
        }
    }
private:
    HPSS snapshot_{};
    decltype(&PssFreeSnapshot) free_{};
    DWORD error_{};
};
}
