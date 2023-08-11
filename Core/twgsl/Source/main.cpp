#include <tint/tint.h>

#include <cstdlib>
#include <array>
#include <iostream>

extern "C"
{
void spirv_to_wgsl(const void* bytes, int length);
void wgsl_to_spirv(const void* fn, int fnlen, const void* bytes, int bytelen);
    
// Callback to JavaScript
extern void return_string(const void* data, int length);
extern void return_entrypoint(int stage, const void* data, int length);
extern void return_error(const void* data, int length);
}

static inline const char* convertMessageType(tint::diag::Severity severity)
{
    switch (severity) {
    case tint::diag::Severity::Note:
        return "note";
    case tint::diag::Severity::Warning:
        return "warning";
    case tint::diag::Severity::Error:
        return "error";
    case tint::diag::Severity::InternalCompilerError:
        return "internal_error";
    case tint::diag::Severity::Fatal:
        return "fatal";
    }
    abort();
}

static inline std::string formatTintDiag(const tint::diag::Diagnostic& diag)
{
    char buf[16384];
    snprintf(buf, sizeof(buf), "%s: %s at %s:%zu:%zu",
             convertMessageType(diag.severity),
             diag.message.c_str(),
             diag.source.file ? diag.source.file->path.c_str() : "<no filename>",
             diag.source.range.begin.line,
             diag.source.range.begin.column);
    return std::string(buf);
}

void spirv_to_wgsl(const void* bytes, int length) {
    std::vector<uint32_t> spirv{};
    spirv.resize(length / sizeof(uint32_t));
    std::memcpy(spirv.data(), bytes, length);

    tint::Program program{tint::reader::spirv::Parse(spirv)};

    if (!program.IsValid()) {
        std::string msg;
        for (const auto& message : program.Diagnostics())
        {
            if (!msg.empty())
                msg += "\n";
            msg += formatTintDiag(message);
        }
        return_error(msg.data(), msg.size());
        return;
    }
    
    tint::writer::wgsl::Options options{};
    auto result = tint::writer::wgsl::Generate(&program, options);

    return_string(result.wgsl.data(), result.wgsl.size());
}

void wgsl_to_spirv(const void* fn, int fnlen, const void* bytes, int bytelen) {
    std::string wgsl{};
    wgsl.resize(bytelen);
    std::memcpy(&wgsl[0], bytes, bytelen);

    std::string filename;
    if (fnlen > 0) {
        filename.resize(fnlen);
        std::memcpy(&filename[0], fn, fnlen);
    } else {
        filename = "<no filename>";
    }

    tint::Source::File sourceFile(filename, wgsl);
    auto program = tint::reader::wgsl::Parse(&sourceFile);

    if (!program.IsValid()) {
        std::string msg;
        for (const auto& message : program.Diagnostics())
        {
            if (!msg.empty())
                msg += "\n";
            msg += formatTintDiag(message);
        }
        return_error(msg.data(), msg.size());
        return;
    }

    tint::inspector::Inspector inspector(&program);
    const auto& entryPoints = inspector.GetEntryPoints();
    for (const auto& entryPoint : entryPoints) {
        tint::transform::Manager transform_manager;
        tint::transform::DataMap transform_inputs;

        transform_manager.append(std::make_unique<tint::transform::SingleEntryPoint>());
        transform_inputs.Add<tint::transform::SingleEntryPoint::Config>(entryPoint.name);

        auto out = transform_manager.Run(&program, std::move(transform_inputs));
        if (!out.program.IsValid()) {
            std::string msg;
            for (const auto& message : out.program.Diagnostics())
            {
                if (!msg.empty())
                    msg += "\n";
                msg += formatTintDiag(message);
            }
            return_error(msg.data(), msg.size());
            return;
        }

        tint::writer::spirv::Options options;
        options.emit_vertex_point_size = true;
        options.disable_workgroup_init = true;
        options.disable_robustness = true;
        // options.clamp_frag_depth = true;

        // generate spirv from wgsl
        auto result = tint::writer::spirv::Generate(&out.program, options);
        if (!result.success) {
            return_error(result.error.data(), result.error.size());
            return;
        }

        return_entrypoint(static_cast<int>(entryPoint.stage), result.spirv.data(), result.spirv.size() * sizeof(uint32_t));
    }
}
