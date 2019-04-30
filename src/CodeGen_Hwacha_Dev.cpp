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
#include "LLVM_Output.h"

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

    device_args = args;

    std::cout << "In CodeGen_Hwacha_Dev::add_kernel\n";

    // Now deduce the types of the arguments to our function
    vector<llvm::Type *> arg_types(args.size());
    for (size_t i = 0; i < args.size(); i++) {
      std::cout << args[i].name << "\n";
        if (args[i].is_buffer) {
            arg_types[i] = llvm_type_of(UInt(8))->getPointerTo();
        } else {
            arg_types[i] = llvm_type_of(args[i].type);
        }
    }

    device_arg_types = arg_types;

    // Make our function
    FunctionType *func_t = FunctionType::get(void_t, arg_types, false);
    std::cout << "creating function with name" << name << "\n";
    function = llvm::Function::Create(func_t, llvm::Function::ExternalLinkage, name, nullptr);
    std::string Str1;
    raw_string_ostream OS1(Str1);
    OS1 << *function;
    OS1.flush();
    std::cout << Str1;

    set_function_attributes_for_target(function, target);

    // Mark the buffer args as no alias
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            function->addParamAttr(i, Attribute::NoAlias);
        }
    }

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

    // Ok, we have a module, function, context, and a builder
    // pointing at a brand new basic block. We're good to go.
    stmt.accept(this);

    // Now we need to end the function
    builder->CreateRetVoid();

    module->getFunctionList().push_back(function);

    // Now verify the function is ok
    verifyFunction(*function);

    // // Finally, verify the module is ok
    verifyModule(*module);

    std::cout << "Done generating llvm bitcode for Hwacha\n";

    // Clear the symbol table
    for (size_t i = 0; i < arg_sym_names.size(); i++) {
        sym_pop(arg_sym_names[i]);
    }
}

void CodeGen_Hwacha_Dev::init_module() {
    init_context();

    #ifdef WITH_HWACHA
    module = get_initial_module_for_hwacha_device(target, context);
    #endif
}

void CodeGen_Hwacha_Dev::set_module(std::unique_ptr<llvm::Module> mod,
                                      llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> *build) {

  module = std::move(mod);
  builder = build;
}



void CodeGen_Hwacha_Dev::visit(const Call *op) {
  //internal_assert(false) << "Not implemented\n";
    // if (op->is_intrinsic(Call::gpu_thread_barrier)) {
    //     llvm::Function *barrier0 = module->getFunction("llvm.nvvm.barrier0");
    //     internal_assert(barrier0) << "Could not find PTX barrier intrinsic (llvm.nvvm.barrier0)\n";
    //     builder->CreateCall(barrier0);
    //     value = ConstantInt::get(i32_t, 0);
    // } else {
         CodeGen_LLVM::visit(op);
    // }
}

string CodeGen_Hwacha_Dev::simt_intrinsic(const string &name) {
  //    internal_assert(false) << "Not implemented\n";
    if (ends_with(name, ".__thread_id_x")) {
      return "llvm.hwacha.veidx";
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.tid.x";
    } else if (ends_with(name, ".__thread_id_y")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.tid.y";
    } else if (ends_with(name, ".__thread_id_z")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.tid.z";
    } else if (ends_with(name, ".__thread_id_w")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.tid.w";
    } else if (ends_with(name, ".__block_id_x")) {
      return "llvm.hwacha.veidx";
        return "llvm.nvvm.read.ptx.sreg.ctaid.x";
    } else if (ends_with(name, ".__block_id_y")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.ctaid.y";
    } else if (ends_with(name, ".__block_id_z")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.ctaid.z";
    } else if (ends_with(name, ".__block_id_w")) {
      internal_assert(false) << "Not implemented\n";
        return "llvm.nvvm.read.ptx.sreg.ctaid.w";
    }
    internal_error << "simt_intrinsic called on bad variable name\n";
    return "";
}

llvm::Function* CodeGen_Hwacha_Dev::add_stripmine_loop(Stmt stmt, const std::string &name) {
    std::cout << "In add_stripmine\n";

    llvm::Function* oldFunction = function;

    FunctionType *func_t = FunctionType::get(void_t, device_arg_types, false);
    function = llvm::Function::Create(func_t, llvm::Function::ExternalLinkage, name, module.get());
    function->addFnAttr(Attribute::NoInline);


    llvm::Metadata *md_args[] = {
      llvm::ValueAsMetadata::get(function)
    };
    llvm::MDNode *md_node = llvm::MDNode::get(*context, md_args);
    module->getOrInsertNamedMetadata("opencl.kernels")->addOperand(md_node);

    // Mark the buffer args as no alias
    for (size_t i = 0; i < device_args.size(); i++) {
        if (device_args[i].is_buffer) {
            function->addParamAttr(i, Attribute::NoAlias);
        }
    }


    // Put the arguments in the symbol table
    vector<string> arg_sym_names;
    {
        size_t i = 0;
        for (auto &fn_arg : function->args()) {

            string arg_sym_name = device_args[i].name;
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

    // Ok, we have a module, function, context, and a builder
    // pointing at a brand new basic block. We're good to go.
    Expr simt_idx = Cast::make(Int(32), Call::make(Int(64), simt_intrinsic(name), std::vector<Expr>(), Call::Extern));
    sym_push(name, codegen(simt_idx));
    stmt.accept(this);

    // Now we need to end the function
    builder->CreateRetVoid();

    // Now verify the function is ok
    verifyFunction(*function);

    std::cout << "Done generating llvm bitcode for Hwacha\n";

    // Clear the symbol table
    for (size_t i = 0; i < arg_sym_names.size(); i++) {
        sym_pop(arg_sym_names[i]);
    }
    sym_pop(name);

    llvm::Function* r = function;
    function = oldFunction;

    return r;
}

void CodeGen_Hwacha_Dev::visit(const For *loop) {
    if (is_gpu_var(loop->name)) {
      if (ends_with(loop->name, "__block_id_x")) {
        internal_assert(ends_with(loop->name, "__block_id_x")) << "Not supported variable " << loop->name << "\n";

        internal_assert(is_zero(loop->min));
        // Value* min = codegen(loop->min);
        // Value* extent = codegen(loop->extent);
        // builder->CreateNSWAdd(min, extent);

        BasicBlock* curr_point = builder->GetInsertBlock();
        llvm::Function* stripmine = add_stripmine_loop(loop->body, loop->name);
        builder->SetInsertPoint(curr_point);

        std::vector<llvm::Value*> args;
        for (size_t i = 0 ; i < device_args.size(); i++) {
          args.push_back(sym_get(device_args[i].name));
        }
        builder->CreateCall(stripmine, args);
      } else if (ends_with(loop->name, "__thread_id_x")) {
        codegen(loop->body);
      } else {
        internal_error << "unsupported\n";
      }


      // if (ends_with(loop->name, "__block_id_x")) {
      //   internal_assert(ends_with(loop->name, "__block_id_x")) << "Not supported variable " << loop->name << "\n";
      //   std::cout << "visit For: Trying to make hwacha\n";
      //   std::cout << loop->name << "\n";

      //   Expr simt_idx = Cast::make(Int(32), Call::make(Int(64), simt_intrinsic(loop->name), std::vector<Expr>(), Call::Extern));
      //   internal_assert(is_zero(loop->min));
      //   sym_push(loop->name, codegen(simt_idx));


      //   codegen(loop->body);


      //   sym_pop(loop->name);
      // } else if (ends_with(loop->name, "__thread_id_x")) {
      //   codegen(loop->body);
      // } else {
      //   internal_error << "unsupported\n";
      // }
    } else {
        CodeGen_LLVM::visit(loop);
    }
    std::cout << "visit For: done\n";
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
  std::cout << "visiting hwacha load\n";
  //internal_assert(false) << "Not implemented\n";
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

    CodeGen_LLVM::visit(op);
    std::cout << "done visiting hwacha load\n";
}

void CodeGen_Hwacha_Dev::visit(const Store *op) {
  std::cout << "visiting hwacha store\n";
  //internal_assert(false) << "Not implemented\n";
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

    CodeGen_LLVM::visit(op);
    std::cout << "done visiting hwacha store\n";
}

string CodeGen_Hwacha_Dev::march() const {
    return "";
}

string CodeGen_Hwacha_Dev::mcpu() const {
    return "";
}

string CodeGen_Hwacha_Dev::mattrs() const {
    return "+x";
}

bool CodeGen_Hwacha_Dev::use_soft_float_abi() const {
    return true;
}

std::unique_ptr<llvm::Module> CodeGen_Hwacha_Dev::get_module() {
  return std::move(module);

}

vector<char> CodeGen_Hwacha_Dev::compile_to_src() {

  std::cout << "compiling hwacha to src\n";
    #ifdef WITH_HWACHA

  std::cout       << "In CodeGen_Hwacha_Dev::compile_to_src\n";

    // DISABLED - hooked in here to force PrintBeforeAll option - seems to be the only way?
    /*char* argv[] = { "llc", "-print-before-all" };*/
    /*int argc = sizeof(argv)/sizeof(char*);*/
    /*cl::ParseCommandLineOptions(argc, argv, "Halide PTX internal compiler\n");*/

    llvm::Triple triple(module->getTargetTriple());

    // Allocate target machine
    std::cout << "making target machine\n";
    std::string err_str;
    const llvm::Target *target = TargetRegistry::lookupTarget(triple.str(), err_str);
    internal_assert(target) << err_str << "\n";
    std::cout << "making target options\n";

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
                                                   llvm::Reloc::Static,
                                                   llvm::CodeModel::Small,
                                                   CodeGenOpt::Aggressive));

    internal_assert(target_machine.get()) << "Could not allocate target machine!";

    module->setDataLayout(target_machine->createDataLayout());

    std::cout << "setting up passes\n";
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
    std::cout << "running passes\n" << std::flush;
    // Run optimization passes
    function_pass_manager.doInitialization();
    for (llvm::Module::iterator i = module->begin(); i != module->end(); i++) {
        function_pass_manager.run(*i);
    }
    function_pass_manager.doFinalization();
    module_pass_manager.run(*module);

    //    if (debug::debug_level() >= 2) {
    //        dump();
        // }
    std::cout << "Done with CodeGen_Hwacha_Dev::compile_to_src\n" << std::flush;

    std::cout << "Hwacha kernel:\n" << outstr.c_str() << std::flush;
    std::cout << "\ndone hwacha kernel\n" << std::flush;

    vector<char> buffer(outstr.begin(), outstr.end());
    std::cout << "returning\n" << std::flush;

    std::ofstream out("hwacha.s");
    out << outstr.c_str();
    out.close();

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
// Str now contains the module text
    module->print(dbgs(), nullptr, false, true);

}

std::string CodeGen_Hwacha_Dev::print_gpu_name(const std::string &name) {
    return name;
}

}  // namespace Internal
}  // namespace Halide
