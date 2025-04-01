#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>

#include "UnitLICM.h"
#include "UnitLoopInfo.h"
#include "UnitSCCP.h"


/// Registers the three passes for this project with LLVM's pass mananger
llvm::PassPluginLibraryInfo getUnitProjectPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "ECE479K Lab 3", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                // Register LoopId
                PB.registerAnalysisRegistrationCallback(
                        [](FunctionAnalysisManager &FAM)
                        {
                            FAM.registerPass(
                                    [&]
                                    { return ece479k::UnitLoopAnalysis(); });
                        });
                // Register LICM
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                           ArrayRef<PassBuilder::PipelineElement>)
                        {
                            if (Name == "unit-licm")
                            {
                                FPM.addPass(LoopSimplifyPass()); // canonicalize loops
                                FPM.addPass(LCSSAPass()); // close loops
                                FPM.addPass(ece479k::UnitLICM());
                                return true;
                            }
                            return false;
                        });

                // Register SCCP
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                           ArrayRef<PassBuilder::PipelineElement>)
                        {
                            if (Name == "unit-sccp")
                            {
                                FPM.addPass(ece479k::UnitSCCP());
                                return true;
                            }
                            return false;
                        });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return getUnitProjectPluginInfo();
}
