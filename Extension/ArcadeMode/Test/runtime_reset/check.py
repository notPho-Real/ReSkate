"""Compile actual reset/dispatch bodies against finite test doubles; no native calls."""
from pathlib import Path
import argparse
import hashlib
import os
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parents[4]
EXPECTED = {'persist':'reset persisted preference or ran more than once', 'initial_load':'pending preference initialization admitted', 'expiry':'expired receipt admitted', 'owner':'wrong engine thread admitted', 'restore_mode':'qualified reset rejected', 'cleanup_scope':'completed operation retained ownership'}

def body(source, marker):
    assert source.count(marker) == 1, marker
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), source)
    start = source.index(marker)
    opening = masked.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return source[start:end]

def fixture(rollback):
    mode = (ROOT / 'Extension/ArcadeMode/arcade_mode.cpp').read_text()
    bridge = (ROOT / 'Runtime/console_bridge.cpp').read_text()
    operation = body(mode, 'bool reset_runtime_transient() noexcept')
    adapter = body(bridge, 'std::string apply_console_arcade_runtime_reset()')
    scope = body(bridge, 'struct ConsoleResetScope') + ';'
    assert 'set_enabled(original_mode, false);' in operation
    assert 'set_local_preference(' not in operation
    assert 'ConsoleResetScope reset_scope{queued_at}' in bridge
    assert 'execute_console_command(console_command->text, console_command->queued_at)' in (ROOT / 'Runtime/client_tick.cpp').read_text()
    assert 'request_arcade_runtime_reset()' in (ROOT / 'Extension/Console/commands.cpp').read_text()
    if rollback == 'persist': operation = operation.replace('set_enabled(original_mode, false);', 'set_enabled(original_mode, true);')
    if rollback == 'initial_load': operation = operation.replace(' || !preference_loaded', '')
    if rollback == 'expiry': adapter = adapter.replace('reset_now - *console_reset_queued_at > std::chrono::milliseconds(1000)', 'false')
    if rollback == 'owner': adapter = adapter.replace('r.engine_thread.load() != GetCurrentThreadId()', 'false')
    if rollback == 'restore_mode': operation = operation.replace('set_enabled(original_mode, false);', 'set_enabled(false, false);')
    if rollback == 'cleanup_scope': scope = scope.replace('console_reset_queued_at.reset();', '')
    # Only the external clock primitive is substituted. All production guard,
    # reset, output and cleanup statements remain source-extracted verbatim.
    adapter = adapter.replace('std::chrono::steady_clock::now()', 'FixtureClock::now()')
    return r'''
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
using Clock = std::chrono::steady_clock;
struct FixtureClock { static Clock::time_point value; static Clock::time_point now() { return value; } };
Clock::time_point FixtureClock::value{std::chrono::seconds(10)};
unsigned checks{};
void require(bool value, const char* reason) { ++checks; if(!value) throw std::runtime_error(reason); }
namespace dingosdk::arcade_mode {
std::atomic<bool> ready{true}; bool preference_loaded{true}, mode{true};
unsigned stored_preference{4}; std::uint64_t store_revision{406};
unsigned resets{}, writes{}; bool routes{}, packets{}, commands{}, native_context{};
bool enabled() noexcept {return mode;}
void set_enabled(bool value, bool persist) noexcept { ++resets; writes += persist ? 1u : 0u; if(persist){stored_preference=value?2u:1u;++store_revision;} mode=value; routes=packets=commands=native_context=false; }
''' + operation + r'''
}
struct Scheduler { bool busy{}, console{true}; unsigned thread{7}; bool loading() const{return busy;} bool executing_console(unsigned t) const{return console && thread==t;} };
struct Runtime { std::mutex mutex; std::atomic<unsigned> engine_thread{7}; bool native_tick_ready{true}, observer_failed{}; Scheduler requests; };
Runtime instance;
Runtime& runtime() {return instance;}
unsigned caller{7}; unsigned GetCurrentThreadId(){return caller;}
thread_local std::optional<Clock::time_point> console_reset_queued_at;
''' + scope + '\n' + adapter + r'''
void baseline(bool on=true) {
 using namespace dingosdk::arcade_mode;
 ready=true; preference_loaded=true; mode=on; resets=writes=0; routes=packets=commands=native_context=true;
 caller=7;instance.engine_thread=7;instance.native_tick_ready=true;instance.observer_failed=false;instance.requests={};
 console_reset_queued_at=FixtureClock::value;
}
void rejected(const char* why) {
 const auto result=apply_console_arcade_runtime_reset();
 require(result.starts_with("error:"),why);
 require(dingosdk::arcade_mode::resets==0,"rejected request mutated transient state");
 require(dingosdk::arcade_mode::writes==0,"rejected request wrote preference");
}
int main() {try {
 using namespace dingosdk::arcade_mode;
 for(bool on : {false,true}) {
  baseline(on); const auto result=apply_console_arcade_runtime_reset();
  require(!result.starts_with("error:"),"qualified reset rejected");
  require(mode==on,"exact original runtime mode not preserved");
  require(resets==1 && writes==0,"reset persisted preference or ran more than once");
  require(!routes&&!packets&&!commands&&!native_context,"transient cleanup incomplete");
  require(result.find(on?"mode ON":"mode OFF")!=std::string::npos,"actual runtime readback incorrect");
 }
 // Every preference representation is observational only: the reset has no
 // Store access and its sole mode call explicitly disables persistence.
 for(unsigned preference_state=0;preference_state<5;++preference_state) {
  for(bool on : {false,true}) {
   baseline(on); stored_preference=preference_state; store_revision=406;
   apply_console_arcade_runtime_reset();
   require(writes==0 && stored_preference==preference_state && store_revision==406,"preference representation changed");
   require(mode==on,"preference representation overrode runtime value");
  }
 }
 baseline(); preference_loaded=false; rejected("pending preference initialization admitted");
 // A refused pre-initialization operation cannot mask the actual later load.
 mode=false; preference_loaded=true;
 require(!apply_console_arcade_runtime_reset().starts_with("error:") && !mode,"post-load reset restored a stale runtime value");
 baseline(); ready=false; rejected("runtime not ready admitted");
 baseline(); instance.engine_thread=8; rejected("wrong engine thread admitted");
 baseline(); instance.requests.thread=8; rejected("wrong dispatch owner admitted");
 baseline(); instance.requests.console=false; rejected("unscheduled call admitted");
 baseline(); instance.requests.busy=true; rejected("loading session admitted");
 baseline(); instance.observer_failed=true; rejected("unhealthy session admitted");
 baseline(); instance.native_tick_ready=false; rejected("unready native tick admitted");
 baseline(); console_reset_queued_at.reset(); rejected("missing request receipt admitted");
 baseline(); console_reset_queued_at=FixtureClock::value+std::chrono::milliseconds(1); rejected("future receipt admitted");
 baseline(); console_reset_queued_at=FixtureClock::value-std::chrono::milliseconds(1001); rejected("expired receipt admitted");
 baseline(); console_reset_queued_at=FixtureClock::value-std::chrono::milliseconds(1000);
 require(!apply_console_arcade_runtime_reset().starts_with("error:"),"exact deadline rejected");
 baseline(); console_reset_queued_at.reset();
 { const ConsoleResetScope active{FixtureClock::value}; require(console_reset_queued_at.has_value(),"scope has no receipt"); }
 require(!console_reset_queued_at,"completed operation retained ownership");
 try { const ConsoleResetScope active{FixtureClock::value}; throw std::runtime_error("fixture"); } catch(const std::runtime_error&) {}
 require(!console_reset_queued_at,"exception retained ownership");
 // No delayed restore exists: an ordinary subsequent mode change wins.
 baseline(); apply_console_arcade_runtime_reset(); set_enabled(false,true);
 require(!mode && writes==1,"ordinary saved mode change was overridden");
 std::cout << checks << " reset/dispatch checks passed\n";
 } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;} }
'''

def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',default=os.environ.get('CXX','clang++'))
    p.add_argument('--rollback',choices=tuple(EXPECTED))
    p.add_argument('--emit',type=Path)
    p.add_argument('--expect-binary',type=Path)
    args=p.parse_args()
    if args.expect_binary:
        result=subprocess.run([str(args.expect_binary)],capture_output=True,text=True)
        assert args.rollback and result.returncode==1 and result.stderr.strip()==EXPECTED[args.rollback], (result.returncode,result.stderr)
        print('Intended rollback assertion: '+result.stderr.strip()); return
    source=fixture(args.rollback)
    if args.emit:
        args.emit.parent.mkdir(parents=True,exist_ok=True); args.emit.write_text(source); return
    with tempfile.TemporaryDirectory(prefix='runtime-reset-') as folder:
        path=Path(folder); cpp=path/'fixture.cpp'; cpp.write_text(source)
        cl=Path(args.compiler).name.lower() in ('cl','cl.exe')
        exe=path/('fixture.exe' if cl else 'fixture')
        flags=['/nologo','/std:c++20','/EHsc','/W4','/WX',str(cpp),'/Fe:'+str(exe)] if cl else ['-std=c++20','-Wall','-Wextra','-Wshadow','-Wconversion','-Werror','-pedantic','-fsanitize=address,undefined',str(cpp),'-o',str(exe)]
        subprocess.run([args.compiler,*flags],cwd=path,check=True)
        result=subprocess.run([str(exe)],cwd=path,capture_output=True,text=True)
        print(result.stdout+result.stderr,end='')
        if args.rollback:
            assert result.returncode==1 and result.stderr.strip()==EXPECTED[args.rollback], (result.returncode,result.stderr)
        else: assert result.returncode==0
if __name__=='__main__': main()
