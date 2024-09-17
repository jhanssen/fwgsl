#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <tint/tint.h>

#include <cstdlib>
#include <array>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

using namespace emscripten;

static inline const char* convertMessageType(tint::diag::Severity severity)
{
    switch (severity) {
    case tint::diag::Severity::Note:
        return "note";
    case tint::diag::Severity::Warning:
        return "warning";
    case tint::diag::Severity::Error:
        return "error";
    }
    abort();
}

static inline std::string formatTintDiag(const tint::diag::Diagnostic& diag)
{
    char buf[16384];
    snprintf(buf, sizeof(buf), "%s: %s at %s:%u:%u",
             convertMessageType(diag.severity),
             diag.message.Plain().c_str(),
             diag.source.file ? diag.source.file->path.c_str() : "<no filename>",
             diag.source.range.begin.line,
             diag.source.range.begin.column);
    return std::string(buf);
}

class EntryPoint
{
public:
    EntryPoint(int st, std::vector<uint32_t>&& sp) : mStage(st), mSpirv(std::move(sp)) { }

    int stage() const { return mStage; }
    val spirv() const { return val(typed_memory_view(mSpirv.size(), mSpirv.data())); }

    int mStage;
    std::vector<uint32_t> mSpirv;
};

class Transpiler
{
public:
    Transpiler() { }

    void wgslToSpirv(const std::string& filename, const std::string& wgsl);

    bool hasError() const { return mError.has_value(); }
    std::string error() const { return mError.value(); }
    uint32_t numEntryPoints() const { return mEntryPoints.size(); }
    EntryPoint entryPoint(uint32_t idx) const { return mEntryPoints[idx]; }

    std::optional<std::string> mError;
    std::vector<EntryPoint> mEntryPoints;
};

void Transpiler::wgslToSpirv(const std::string& filename, const std::string& wgsl)
{
    tint::Source::File sourceFile(filename, wgsl);
    tint::wgsl::reader::Options programOptions = {};
    programOptions.allowed_features.extensions.insert(tint::wgsl::Extension::kChromiumExperimentalFramebufferFetch);
    auto program = tint::wgsl::reader::Parse(&sourceFile);

    mError = {};
    mEntryPoints.clear();

    if (!program.IsValid()) {
        std::string msg;
        for (const auto& message : program.Diagnostics())
        {
            if (!msg.empty())
                msg += "\n";
            msg += formatTintDiag(message);
        }
        mError = std::move(msg);
        return;
    }

    tint::inspector::Inspector inspector(program);
    const auto& entryPoints = inspector.GetEntryPoints();
    for (const auto& entryPoint : entryPoints) {
        tint::ast::transform::Manager transform_manager;
        tint::ast::transform::DataMap transform_inputs, transform_outputs;

        transform_manager.append(std::make_unique<tint::ast::transform::SingleEntryPoint>());
        transform_inputs.Add<tint::ast::transform::SingleEntryPoint::Config>(entryPoint.name);

        auto outProgram = transform_manager.Run(program, std::move(transform_inputs), transform_outputs);
        if (!outProgram.IsValid()) {
            std::string msg;
            for (const auto& message : outProgram.Diagnostics())
            {
                if (!msg.empty())
                    msg += "\n";
                msg += formatTintDiag(message);
            }
            mError = std::move(msg);
            return;
        }

        auto ir = tint::wgsl::reader::ProgramToLoweredIR(outProgram);
        if (ir != tint::Success) {
            std::string msg;
            for (const auto& message : ir.Failure().reason) {
                if (!msg.empty())
                    msg += "\n";
                msg += formatTintDiag(message);
            }
            mError = std::move(msg);
            return;
        }

        tint::spirv::writer::Options options;
        options.emit_vertex_point_size = true;
        options.disable_workgroup_init = true;
        options.disable_robustness = true;
        // options.clamp_frag_depth = true;

        // generate spirv from wgsl
        auto result = tint::spirv::writer::Generate(ir.Get(), options);
        if (result != tint::Success) {
            std::string msg;
            for (const auto& message : result.Failure().reason) {
                if (!msg.empty())
                    msg += "\n";
                msg += formatTintDiag(message);
            }
            mError = std::move(msg);
            return;
        }
        auto output = result.Move();

        mEntryPoints.push_back(EntryPoint(static_cast<int>(entryPoint.stage), std::move(output.spirv)));
    }
}

EMSCRIPTEN_BINDINGS(fwgslwasm) {
    class_<Transpiler>("Transpiler")
    .constructor<>()
    .function("wgslToSpirv", &Transpiler::wgslToSpirv)
    .function("hasError", &Transpiler::hasError)
    .function("error", &Transpiler::error)
    .function("numEntryPoints", &Transpiler::numEntryPoints)
    .function("entryPoint", &Transpiler::entryPoint);
    class_<EntryPoint>("EntryPoint")
    .function("stage", &EntryPoint::stage)
    .function("spirv", &EntryPoint::spirv);
}
