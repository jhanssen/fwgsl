#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <tint/tint.h>
#include <src/tint/lang/core/builtin_fn.h>
#include <src/tint/lang/wgsl/reader/reader.h>
#include <src/tint/lang/wgsl/writer/writer.h>
#include <src/tint/lang/wgsl/program/clone_context.h>
#include <src/tint/lang/wgsl/program/program_builder.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/lang/wgsl/ast/module.h>
#include <src/tint/lang/wgsl/ast/enable.h>
#include <src/tint/lang/wgsl/ast/function.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/identifier_expression.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/ast/transform/data.h>
#include <src/tint/lang/wgsl/ast/transform/manager.h>
#include <src/tint/lang/wgsl/ast/transform/transform.h>
#include <src/tint/lang/wgsl/resolver/resolve.h>
#include <src/tint/lang/wgsl/sem/array.h>
#include <src/tint/lang/wgsl/sem/function.h>
#include <src/tint/lang/wgsl/sem/variable.h>
#include <src/tint/lang/spirv/reader/reader.h>
#include <src/tint/lang/spirv/writer/writer.h>
#include <src/tint/utils/diagnostic/source.h>
#include <src/tint/utils/rtti/castable.h>
#include <src/tint/utils/rtti/switch.h>

#include <cstdlib>
#include <array>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

using namespace emscripten;

class FragColorInputToInputAttachment final : public tint::Castable<FragColorInputToInputAttachment, tint::ast::transform::Transform>
{
public:
    FragColorInputToInputAttachment(std::optional<std::vector<uint32_t>>* convertedColorInput)
        : mConvertedColorInput(convertedColorInput)
    {
    }
    virtual ~FragColorInputToInputAttachment() override = default;

    virtual ApplyResult Apply(const tint::Program& src,
                              const tint::ast::transform::DataMap& inputs,
                              tint::ast::transform::DataMap& outputs) const override
    {
        tint::ProgramBuilder b;
        tint::program::CloneContext ctx{&b, &src, /* auto_clone_symbols */ true};

        bool inputEnabled = false;

        for (auto* decl : src.AST().GlobalDeclarations()) {
            Switch(
                decl,
                [&](const tint::ast::TypeDecl* ty) {
                    b.AST().AddTypeDecl(ctx.Clone(ty));
                },
                [&](const tint::ast::Override* override) {
                    b.AST().AddGlobalVariable(ctx.Clone(override));
                },
                [&](const tint::ast::Var* var) {
                    b.AST().AddGlobalVariable(ctx.Clone(var));
                },
                [&](const tint::ast::Const* c) {
                    b.AST().AddGlobalVariable(ctx.Clone(c));
                },
                [&](const tint::ast::Function* func) {
                    for (const auto& param : func->params) {
                        for (const auto& attr : param->attributes) {
                            if (attr->Is<tint::ast::ColorAttribute>()) {
                                const auto& paramName = param->name->symbol.Name();
                                const auto& paramType = param->type.expr->identifier->symbol.Name();

                                uint32_t idx = 0;

                                const auto& colorAttr = attr->As<tint::ast::ColorAttribute>();
                                if (!colorAttr->expr->Is<tint::ast::IntLiteralExpression>()) {
                                    //fprintf(stderr, "expected int literal for ColorAttribute\n");
                                    return;
                                }
                                idx = colorAttr->expr->As<tint::ast::IntLiteralExpression>()->value;

                                if (!mConvertedColorInput->has_value()) {
                                    *mConvertedColorInput = std::vector<uint32_t>();
                                }
                                mConvertedColorInput->value().push_back(idx);

                                // we need to rewrite this param
                                //needsRewrite = true;
                                ctx.Remove(func->params, param);

                                if (!inputEnabled) {
                                    inputEnabled = true;
                                    b.AST().AddEnable(b.Enable(tint::wgsl::Extension::kChromiumInternalInputAttachments));
                                }

                                // add a global input attachment
                                b.AST().AddGlobalVariable(b.Var(
                                                              b.Ident("haltiInput_" + paramName),                        // name
                                                              b.ty.input_attachment(b.ty.f32()),                          // type
                                                              static_cast<const tint::ast::Expression*>(nullptr),                           // declared_address_space
                                                              static_cast<const tint::ast::Expression*>(nullptr),                           // declared_access
                                                              static_cast<const tint::ast::Expression*>(nullptr),                           // initializer
                                                              tint::Vector<const tint::ast::Attribute*, 2>({
                                                                      b.Group(tint::core::u32(idx)),
                                                                      b.Binding(tint::core::u32(idx)),
                                                                      b.InputAttachmentIndex(tint::core::u32(idx))
                                                                  })
                                                              ));
                                // add a load variable to the function body
                                ctx.InsertFront(func->body->statements, b.Decl(
                                                    b.Var(
                                                        b.Ident(paramName),
                                                        b.ty.vec4(b.ty.f32()),
                                                        static_cast<const tint::ast::Expression*>(nullptr),
                                                        static_cast<const tint::ast::Expression*>(nullptr),
                                                        b.Call(tint::core::BuiltinFn::kInputAttachmentLoad, b.Expr(b.Ident("haltiInput_" + paramName))),
                                                        tint::Empty
                                                        )));
                            }
                        }
                    }

                    b.AST().AddFunction(ctx.Clone(func));
                },
                [&](const tint::ast::Enable* ext) {
                    for (auto* e : ext->extensions) {
                        if (e->name == tint::wgsl::Extension::kChromiumExperimentalFramebufferFetch) {
                            if (ext->extensions.Length() > 1) {
                                ctx.Remove(ext->extensions, e);
                            }
                            return;
                        } else if (e->name == tint::wgsl::Extension::kChromiumInternalInputAttachments) {
                            return;
                        }
                    }
                    b.AST().AddEnable(ctx.Clone(ext));
                },
                [&](const tint::ast::Requires*) {
                    // Drop requires directives as they are optional, and it's non-trivial to determine
                    // which features are needed for which entry points.
                },
                [&](const tint::ast::ConstAssert* ca) { b.AST().AddConstAssert(ctx.Clone(ca)); },
                [&](const tint::ast::DiagnosticDirective* d) { b.AST().AddDiagnosticDirective(ctx.Clone(d)); },  //
                TINT_ICE_ON_NO_MATCH);
        }

        return tint::resolver::Resolve(b);
    }

private:
    std::optional<std::vector<uint32_t>>* mConvertedColorInput;
};

TINT_INSTANTIATE_TYPEINFO(FragColorInputToInputAttachment);

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
    std::optional<std::vector<uint32_t>> convertedColorInputs() const { return mConvertedColorInputs; }

    std::optional<std::string> mError;
    std::vector<EntryPoint> mEntryPoints;
    std::optional<std::vector<uint32_t>> mConvertedColorInputs;
};

void Transpiler::wgslToSpirv(const std::string& filename, const std::string& wgsl)
{
    tint::Source::File sourceFile(filename, wgsl);
    tint::wgsl::reader::Options programOptions = {};
    programOptions.allowed_features.extensions.insert(tint::wgsl::Extension::kChromiumExperimentalFramebufferFetch);
    auto program = tint::wgsl::reader::Parse(&sourceFile, programOptions);

    mError = {};
    mEntryPoints.clear();
    mConvertedColorInputs = {};

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

        transform_manager.append(std::make_unique<FragColorInputToInputAttachment>(&mConvertedColorInputs));

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
    .function("convertedColorInputs", &Transpiler::convertedColorInputs)
    .function("numEntryPoints", &Transpiler::numEntryPoints)
    .function("entryPoint", &Transpiler::entryPoint);
    class_<EntryPoint>("EntryPoint")
    .function("stage", &EntryPoint::stage)
    .function("spirv", &EntryPoint::spirv);
}
