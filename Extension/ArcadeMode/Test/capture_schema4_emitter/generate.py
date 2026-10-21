"""Extract the production capture writer for portable byte-level checks."""
import argparse
import hashlib
from pathlib import Path
import re


def function(source: str, marker: str) -> str:
    if source.count(marker) != 1:
        raise ValueError("writer marker is missing or ambiguous")
    start = source.index(marker)
    hidden = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                    lambda match: " " * len(match.group()), source)
    opening = hidden.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        if end == len(hidden):
            raise ValueError("writer source is incomplete")
        depth += (hidden[end] == "{") - (hidden[end] == "}")
        end += 1
    return source[start:end]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    mode = (root / "Extension/ArcadeMode/arcade_mode.cpp").read_text()
    body = function(mode,
                    "bool write_capture_correlation(const CaptureCorrelationEvidence& r) noexcept {")
    recognition = function(mode,
                    "void capture_recognitions(const UnifiedInput::Diagnostic& diagnostic,std::uint64_t delivery_now,const NativeInputFrame& authority)noexcept{")
    semantic = function(mode, "void capture_semantic(CaptureCorrelationEvidence& row,unsigned kind,unsigned clock,")
    render_ack = function(mode, "void capture_render_ack(BalanceHudSource source,std::uint64_t frame,std::uint64_t render_ms) noexcept {")
    capture_row = function(mode, "bool capture_row(const CaptureCorrelationEvidence& row) noexcept {")
    retire = function(mode, "void retire_abandoned_capture(CaptureCorrelationDrain& drain,InputTelemetry& telemetry) noexcept {")
    consume = function(mode, "void consume_capture_row(CaptureCorrelationDrain& drain,InputTelemetry& telemetry,")
    template = (root / "Extension/ArcadeMode/Test/capture_schema4_emitter/fixture.cpp.in").read_text()
    if any(template.count(marker) != 1 for marker in
           ("@@PRODUCTION_WRITER@@", "@@PRODUCTION_RECOGNITION@@", "@@PRODUCTION_SEMANTIC@@", "@@PRODUCTION_RENDER_ACK@@")):
        raise ValueError("fixture writer slot is ambiguous")
    generated = (template.replace("@@PRODUCTION_WRITER@@", body)
                 .replace("@@PRODUCTION_RECOGNITION@@", recognition)
                 .replace("@@PRODUCTION_SEMANTIC@@", semantic)
                 .replace("@@PRODUCTION_RENDER_ACK@@", render_ack))
    output = args.output / "current.cpp"
    teardown_template = (root / "Extension/ArcadeMode/Test/capture_schema4_emitter/teardown.cpp.in").read_text()
    teardown = (teardown_template.replace("@@PRODUCTION_CAPTURE_ROW@@", capture_row)
                .replace("@@PRODUCTION_RETIRE@@", retire)
                .replace("@@PRODUCTION_CONSUME@@", consume))
    teardown_output = args.output / "teardown.cpp"
    if args.check:
        if output.read_text() != generated:
            raise ValueError("generated writer differs from current production source")
        if teardown_output.read_text() != teardown:
            raise ValueError("generated teardown differs from current production source")
        print("Capture writer source correspondence", hashlib.sha256(body.encode()).hexdigest())
        print("Capture recognition source correspondence", hashlib.sha256(recognition.encode()).hexdigest())
        print("Capture semantic source correspondence", hashlib.sha256(semantic.encode()).hexdigest())
        print("Capture render acknowledgement source correspondence", hashlib.sha256(render_ack.encode()).hexdigest())
        print("Capture teardown source correspondence", hashlib.sha256((capture_row+retire+consume).encode()).hexdigest())
    else:
        args.output.mkdir(parents=True, exist_ok=True)
        output.write_text(generated)
        teardown_output.write_text(teardown)


if __name__ == "__main__":
    main()
