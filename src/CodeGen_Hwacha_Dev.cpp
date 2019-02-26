#include "CodeGen_Hwacha_Dev.h"
#include "CodeGen_Internal.h"
#include "Debug.h"
#include "ExprUsesVar.h"
#include "IREquality.h"
#include "IRMatch.h"
#include "IRMutator.h"
#include "IROperator.h"
#include "IRPrinter.h"
#include "LLVM_Headers.h"
#include "LLVM_Runtime_Linker.h"
#include "Simplify.h"
#include "Solve.h"
#include "Target.h"

#include <fstream>


namespace Halide {
namespace Internal {

using std::string;
using std::vector;

using namespace llvm;

CodeGen_Hwacha_Dev::CodeGen_Hwacha_Dev(Target host) : CodeGen_LLVM(host) {
    #if !(WITH_HWACHA)
    user_error << "Hwacha not enabled for this build of Halide.\n";
    #endif
    context = new llvm::LLVMContext();
}

CodeGen_Hwacha_Dev::~CodeGen_Hwacha_Dev() {
    // This is required as destroying the context before the module
    // results in a crash. Really, responsibility for destruction
    // should be entirely in the parent class.
    // TODO: Figure out how to better manage the context -- e.g. allow using
    // same one as the host.
    module.reset();
    delete context;
}

void CodeGen_Hwacha_Dev::add_kernel(Stmt stmt,
                                 const std::string &name,
                                 const std::vector<DeviceArgument> &args) {
    internal_assert(module != nullptr);

    debug(2) << "In CodeGen_Hwacha_Dev::add_kernel\n";

    // Now deduce the types of the arguments to our function
    vector<llvm::Type *> arg_types(args.size());
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            arg_types[i] = llvm_type_of(UInt(8))->getPointerTo();
        } else {
            arg_types[i] = llvm_type_of(args[i].type);
        }
    }

    // Make our function
    FunctionType *func_t = FunctionType::get(void_t, arg_types, false);
    function = llvm::Function::Create(func_t, llvm::Function::ExternalLinkage, name, module.get());
    set_function_attributes_for_target(function, target);

    // Mark the buffer args as no alias
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            function->addParamAttr(i, Attribute::NoAlias);
        }
    }

    // Make the initial basic block
    entry_block = BasicBlock::Create(*context, "entry", function);
    builder->SetInsertPoint(entry_block);

    // Put the arguments in the symbol table
    vector<string> arg_sym_names;
    {
        size_t i = 0;
        for (auto &fn_arg : function->args()) {

            string arg_sym_name = args[i].name;
            sym_push(arg_sym_name, &fn_arg);
            fn_arg.setName(arg_sym_name);
            arg_sym_names.push_back(arg_sym_name);

            i++;
        }
    }

    // We won't end the entry block yet, because we'll want to add
    // some allocas to it later if there are local allocations. Start
    // a new block to put all the code.
    BasicBlock *body_block = BasicBlock::Create(*context, "body", function);
    builder->SetInsertPoint(body_block);

    debug(1) << "Generating llvm bitcode for kernel...\n";
    // Ok, we have a module, function, context, and a builder
    // pointing at a brand new basic block. We're good to go.
    stmt.accept(this);

    // Now we need to end the function
    builder->CreateRetVoid();

    // Make the entry block point to the body block
    builder->SetInsertPoint(entry_block);
    builder->CreateBr(body_block);

    // Add the annotation that it is a kernel function.
    // llvm::Metadata *md_args[] = {
    //     llvm::ValueAsMetadata::get(function),
    //     MDString::get(*context, "kernel"),
    //     llvm::ValueAsMetadata::get(ConstantInt::get(i32_t, 1))
    // };

    //MDNode *md_node = MDNode::get(*context, md_args);

    //module->getOrInsertNamedMetadata("nvvm.annotations")->addOperand(md_node);


    // Now verify the function is ok
    verifyFunction(*function);

    // Finally, verify the module is ok
    verifyModule(*module);

    debug(2) << "Done generating llvm bitcode for Hwacha\n";

    // Clear the symbol table
    for (size_t i = 0; i < arg_sym_names.size(); i++) {
        sym_pop(arg_sym_names[i]);
    }
}

void CodeGen_Hwacha_Dev::init_module() {
    init_context();

    // #ifdef WITH_HWACHA
    // module = get_initial_module_for_hwacha_device(target, context);
    // #endif
}

void CodeGen_Hwacha_Dev::visit(const Call *op) {
    internal_assert(false) << "Not implemented\n";
    // if (op->is_intrinsic(Call::gpu_thread_barrier)) {
    //     llvm::Function *barrier0 = module->getFunction("llvm.nvvm.barrier0");
    //     internal_assert(barrier0) << "Could not find PTX barrier intrinsic (llvm.nvvm.barrier0)\n";
    //     builder->CreateCall(barrier0);
    //     value = ConstantInt::get(i32_t, 0);
    // } else {
    //     CodeGen_LLVM::visit(op);
    // }
}

string CodeGen_Hwacha_Dev::simt_intrinsic(const string &name) {
    internal_assert(false) << "Not implemented\n";
    // if (ends_with(name, ".__thread_id_x")) {
    //     return "llvm.nvvm.read.ptx.sreg.tid.x";
    // } else if (ends_with(name, ".__thread_id_y")) {
    //     return "llvm.nvvm.read.ptx.sreg.tid.y";
    // } else if (ends_with(name, ".__thread_id_z")) {
    //     return "llvm.nvvm.read.ptx.sreg.tid.z";
    // } else if (ends_with(name, ".__thread_id_w")) {
    //     return "llvm.nvvm.read.ptx.sreg.tid.w";
    // } else if (ends_with(name, ".__block_id_x")) {
    //     return "llvm.nvvm.read.ptx.sreg.ctaid.x";
    // } else if (ends_with(name, ".__block_id_y")) {
    //     return "llvm.nvvm.read.ptx.sreg.ctaid.y";
    // } else if (ends_with(name, ".__block_id_z")) {
    //     return "llvm.nvvm.read.ptx.sreg.ctaid.z";
    // } else if (ends_with(name, ".__block_id_w")) {
    //     return "llvm.nvvm.read.ptx.sreg.ctaid.w";
    // }
    // internal_error << "simt_intrinsic called on bad variable name\n";
    return "";
}

void CodeGen_Hwacha_Dev::visit(const For *loop) {
    internal_assert(false) << "Not implemented\n";
    // if (is_gpu_var(loop->name)) {
    //     Expr simt_idx = Call::make(Int(32), simt_intrinsic(loop->name), std::vector<Expr>(), Call::Extern);
    //     internal_assert(is_zero(loop->min));
    //     sym_push(loop->name, codegen(simt_idx));
    //     codegen(loop->body);
    //     sym_pop(loop->name);
    // } else {
    //     CodeGen_LLVM::visit(loop);
    // }
}

void CodeGen_Hwacha_Dev::visit(const Allocate *alloc) {
    user_assert(false) << "Not implemented\n";
    // user_assert(!alloc->new_expr.defined()) << "Allocate node inside Hwacha kernel has custom new expression.\n" <<
    //     "(Memoization is not supported inside GPU kernels at present.)\n";
    // if (alloc->name == "__shared") {
    //     // Hwacha uses zero in address space 3 as the base address for shared memory
    //     Value *shared_base = Constant::getNullValue(PointerType::get(i8_t, 3));
    //     sym_push(alloc->name, shared_base);
    // } else {
    //     debug(2) << "Allocate " << alloc->name << " on device\n";

    //     string allocation_name = alloc->name;
    //     debug(3) << "Pushing allocation called " << allocation_name << " onto the symbol table\n";

    //     // Jump back to the entry and generate an alloca. Note that by
    //     // jumping back we're rendering any expression we carry back
    //     // meaningless, so we had better only be dealing with
    //     // constants here.
    //     int32_t size = alloc->constant_allocation_size();
    //     user_assert(size > 0)
    //         << "Allocation " << alloc->name << " has a dynamic size. "
    //         << "Only fixed-size allocations are supported on the gpu. "
    //         << "Try storing into shared memory instead.";

    //     BasicBlock *here = builder->GetInsertBlock();

    //     builder->SetInsertPoint(entry_block);
    //     Value *ptr = builder->CreateAlloca(llvm_type_of(alloc->type), ConstantInt::get(i32_t, size));
    //     builder->SetInsertPoint(here);
    //     sym_push(allocation_name, ptr);
    // }
    // codegen(alloc->body);
}

void CodeGen_Hwacha_Dev::visit(const Free *f) {
    internal_assert(false) << "Not implemented\n";    
    // sym_pop(f->name);
}

void CodeGen_Hwacha_Dev::visit(const AssertStmt *op) {
    internal_assert(false) << "Not implemented\n";
    // // Discard the error message for now.
    // Expr trap = Call::make(Int(32), "halide_ptx_trap", {}, Call::Extern);
    // codegen(IfThenElse::make(!op->condition, Evaluate::make(trap)));
}

void CodeGen_Hwacha_Dev::visit(const Load *op) {
    internal_assert(false) << "Not implemented\n";
    // // Do aligned 4-wide 32-bit loads as a single i128 load.
    // const Ramp *r = op->index.as<Ramp>();
    // // TODO: lanes >= 4, not lanes == 4
    // if (is_one(op->predicate) && r && is_one(r->stride) && r->lanes == 4 && op->type.bits() == 32) {
    //     ModulusRemainder align = op->alignment;
    //     if (align.modulus % 4 == 0 && align.remainder % 4 == 0) {
    //         Expr index = simplify(r->base / 4);
    //         Expr equiv = Load::make(UInt(128), op->name, index,
    //                                 op->image, op->param, const_true(), align / 4);
    //         equiv = reinterpret(op->type, equiv);
    //         codegen(equiv);
    //         return;
    //     }
    // }

    // CodeGen_LLVM::visit(op);
}

void CodeGen_Hwacha_Dev::visit(const Store *op) {
    internal_assert(false) << "Not implemented\n";
    // // Do aligned 4-wide 32-bit stores as a single i128 store.
    // const Ramp *r = op->index.as<Ramp>();
    // // TODO: lanes >= 4, not lanes == 4
    // if (is_one(op->predicate) && r && is_one(r->stride) && r->lanes == 4 && op->value.type().bits() == 32) {
    //     ModulusRemainder align = op->alignment;
    //     if (align.modulus % 4 == 0 && align.remainder % 4 == 0) {
    //         Expr index = simplify(r->base / 4);
    //         Expr value = reinterpret(UInt(128), op->value);
    //         Stmt equiv = Store::make(op->name, value, index, op->param, const_true(), align / 4);
    //         codegen(equiv);
    //         return;
    //     }
    // }

    // CodeGen_LLVM::visit(op);
}

string CodeGen_Hwacha_Dev::march() const {
    return "";
}

string CodeGen_Hwacha_Dev::mcpu() const {
    return "";
}

string CodeGen_Hwacha_Dev::mattrs() const {
    return "+xhwacha";
}

bool CodeGen_Hwacha_Dev::use_soft_float_abi() const {
    return true;
}

vector<char> CodeGen_Hwacha_Dev::compile_to_src() {

    #ifdef WITH_HWACHA

    debug(2) << "In CodeGen_Hwacha_Dev::compile_to_src";

    // DISABLED - hooked in here to force PrintBeforeAll option - seems to be the only way?
    /*char* argv[] = { "llc", "-print-before-all" };*/
    /*int argc = sizeof(argv)/sizeof(char*);*/
    /*cl::ParseCommandLineOptions(argc, argv, "Halide PTX internal compiler\n");*/

    llvm::Triple triple(module->getTargetTriple());

    // Allocate target machine

    std::string err_str;
    const llvm::Target *target = TargetRegistry::lookupTarget(triple.str(), err_str);
    internal_assert(target) << err_str << "\n";

    TargetOptions options;
    options.PrintMachineCode = false;
    options.AllowFPOpFusion = FPOpFusion::Fast;
    options.UnsafeFPMath = true;
    options.NoInfsFPMath = true;
    options.NoNaNsFPMath = true;
    options.HonorSignDependentRoundingFPMathOption = false;
    options.NoZerosInBSS = false;
    options.GuaranteedTailCallOpt = false;
    options.StackAlignmentOverride = 0;

    std::unique_ptr<TargetMachine>
        target_machine(target->createTargetMachine(triple.str(),
                                                   mcpu(), mattrs(), options,
                                                   llvm::Reloc::PIC_,
                                                   llvm::CodeModel::Small,
                                                   CodeGenOpt::Aggressive));

    internal_assert(target_machine.get()) << "Could not allocate target machine!";

    module->setDataLayout(target_machine->createDataLayout());

    // Set up passes
    llvm::SmallString<8> outstr;
    raw_svector_ostream ostream(outstr);
    ostream.SetUnbuffered();

    legacy::FunctionPassManager function_pass_manager(module.get());
    legacy::PassManager module_pass_manager;

    module_pass_manager.add(createTargetTransformInfoWrapperPass(target_machine->getTargetIRAnalysis()));
    function_pass_manager.add(createTargetTransformInfoWrapperPass(target_machine->getTargetIRAnalysis()));

    PassManagerBuilder b;
    b.OptLevel = 3;
    b.Inliner = createFunctionInliningPass(b.OptLevel, 0, false);
    b.LoopVectorize = true;
    b.SLPVectorize = true;

    target_machine->adjustPassManager(b);

    b.populateFunctionPassManager(function_pass_manager);
    b.populateModulePassManager(module_pass_manager);

    // Override default to generate verbose assembly.
    target_machine->Options.MCOptions.AsmVerbose = true;

    // Output string stream

    // Ask the target to add backend passes as necessary.
#if LLVM_VERSION >= 70
    bool fail = target_machine->addPassesToEmitFile(module_pass_manager, ostream, nullptr,
                                                    TargetMachine::CGFT_AssemblyFile,
                                                    true);
#else
    bool fail = target_machine->addPassesToEmitFile(module_pass_manager, ostream,
                                                    TargetMachine::CGFT_AssemblyFile,
                                                    true);
#endif
    if (fail) {
        internal_error << "Failed to set up passes to emit Hwacha source\n";
    }

    // Run optimization passes
    function_pass_manager.doInitialization();
    for (llvm::Module::iterator i = module->begin(); i != module->end(); i++) {
        function_pass_manager.run(*i);
    }
    function_pass_manager.doFinalization();
    module_pass_manager.run(*module);

    if (debug::debug_level() >= 2) {
        dump();
    }
    debug(2) << "Done with CodeGen_Hwacha_Dev::compile_to_src";

    debug(1) << "Hwacha kernel:\n" << outstr.c_str() << "\n";

    vector<char> buffer(outstr.begin(), outstr.end());
    return buffer;
#else // WITH_Hwacha
    return vector<char>();
#endif
}

int CodeGen_Hwacha_Dev::native_vector_bits() const {
    // Hwacha doesn't really do vectorization. The widest type is a double.
    return 64;
}

string CodeGen_Hwacha_Dev::get_current_kernel_name() {
    return function->getName();
}

void CodeGen_Hwacha_Dev::dump() {
    module->print(dbgs(), nullptr, false, true);
}

std::string CodeGen_Hwacha_Dev::print_gpu_name(const std::string &name) {
    return name;
}

}  // namespace Internal
}  // namespace Halide
