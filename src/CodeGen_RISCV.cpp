#include "CodeGen_RISCV.h"
#include "LLVM_Headers.h"
#include "Util.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;

using namespace llvm;

CodeGen_RISCV::CodeGen_RISCV(Target t) : CodeGen_Posix(t) {
    #if !(WITH_RISCV)
    user_error << "llvm build not configured with RISCV target enabled.\n";
    #endif
    user_assert(llvm_RISCV_enabled) << "llvm build not configured with RISCV target enabled.\n";
}

string CodeGen_RISCV::mcpu() const {
    if (target.bits == 32) {
        return "";
    } else {
        return "";
    }
}

string CodeGen_RISCV::mattrs() const {
    if (target.bits == 32) {
        return "+a,+c,+f,+m";
    } else {
        return "+64bit,+a,+c,+d,+f,+m";
    }
}

bool CodeGen_RISCV::use_soft_float_abi() const {
    // LLVM does not support hard_float ABI for riscv yet
    return true;
}

bool CodeGen_RISCV::use_pic() const {
    return false;
}


int CodeGen_RISCV::native_vector_bits() const {
    return 64;
}

}  // namespace Internal
}  // namespace Halide
