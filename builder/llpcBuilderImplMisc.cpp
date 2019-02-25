/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2019 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
 ***********************************************************************************************************************
 * @file  llpcBuilderImplMisc.cpp
 * @brief LLPC source file: implementation of miscellaneous Builder methods
 ***********************************************************************************************************************
 */
#include "llpcBuilderImpl.h"
#include "llpcContext.h"

#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/InlineAsm.h"

#define DEBUG_TYPE "llpc-builder-impl-misc"

using namespace Llpc;
using namespace llvm;

// =====================================================================================================================
// Create a "kill". Only allowed in a fragment shader.
Instruction* BuilderImplMisc::CreateKill(
    const Twine& instName) // [in] Name to give instruction(s)
{
    // This tells the config builder to set KILL_ENABLE in DB_SHADER_CONTROL.
    // Doing it here is suboptimal, as it does not allow for subsequent middle-end optimizations removing the
    // section of code containing the kill.
    auto pResUsage = getContext().GetShaderResourceUsage(ShaderStageFragment);
    pResUsage->builtInUsage.fs.discard = true;

    return CreateIntrinsic(Intrinsic::amdgcn_kill, {}, getFalse(), nullptr, instName);
}

// =====================================================================================================================
// Create a "readclock".
Instruction* BuilderImplMisc::CreateReadClock(
    bool         realtime,  // Whether to read real-time clock counter
    const Twine& instName)  // [in] Name to give instruction(s)
{
    CallInst* pReadClock = nullptr;
    if (realtime)
    {
        pReadClock = CreateIntrinsic(Intrinsic::amdgcn_s_memrealtime, {}, {}, nullptr, instName);
    }
    else
    {
        pReadClock = CreateIntrinsic(Intrinsic::amdgcn_s_memtime, {}, {}, nullptr, instName);
    }
    pReadClock->addAttribute(AttributeList::FunctionIndex, Attribute::ReadOnly);

    // NOTE: The inline ASM is to prevent optimization of backend compiler.
    InlineAsm* pAsmFunc =
        InlineAsm::get(FunctionType::get(getInt64Ty(), { getInt64Ty() }, false), "; %1", "=r,0", true);

    pReadClock = CreateCall(pAsmFunc, { pReadClock });

    return pReadClock;
}
