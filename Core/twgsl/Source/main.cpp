#include <tint/tint.h>

#include <array>
#include <iostream>

extern "C"
{
void spirv_to_wgsl(const void* bytes, int length);
void wgsl_to_spirv(const void* bytes, int length);
    
// Callback to JavaScript
extern void return_string(const void* data, int length);
extern void return_entrypoint(int stage, const void* data, int length);
}

void spirv_to_wgsl(const void* bytes, int length) {
    std::vector<uint32_t> spirv{};
    spirv.resize(length / sizeof(uint32_t));
    std::memcpy(spirv.data(), bytes, length);

    tint::Program program{tint::reader::spirv::Parse(spirv)};

    for (const auto& message : program.Diagnostics())
    {
        std::cout << message.message << std::endl << std::endl;
    }
    
    tint::writer::wgsl::Options options{};
    auto result = tint::writer::wgsl::Generate(&program, options);

    return_string(result.wgsl.data(), result.wgsl.size());
}

void wgsl_to_spirv(const void* bytes, int length) {
    std::string wgsl{};
    wgsl.resize(length);
    std::memcpy(&wgsl[0], bytes, length);

    tint::Source::File sourceFile("filename", wgsl);
    auto program = tint::reader::wgsl::Parse(&sourceFile);

    tint::inspector::Inspector inspector(&program);
    const auto& entryPoints = inspector.GetEntryPoints();
    for (const auto& entryPoint : entryPoints) {
        tint::transform::Manager transform_manager;
        tint::transform::DataMap transform_inputs;

        transform_manager.append(std::make_unique<tint::transform::SingleEntryPoint>());
        transform_inputs.Add<tint::transform::SingleEntryPoint::Config>(entryPoint.name);

        auto out = transform_manager.Run(&program, std::move(transform_inputs));
        if (!out.program.IsValid()) {
            for (const auto& message : program.Diagnostics())
            {
                std::cout << message.message << std::endl << std::endl;
            }
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
            std::cout << result.error << std::endl << std::endl;
            return;
        }

        return_entrypoint(static_cast<int>(entryPoint.stage), result.spirv.data(), result.spirv.size());
    }
}
