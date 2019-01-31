#include "VcgGen.h"
#include "Filesystem.h"

#include <stdexcept>

using namespace cs241c;

VcgGen::VcgGen(const Module *InputModule) : InputModule(InputModule) {}

void VcgGen::generate(const std::string &OutFilePath) {
  if (fileExists(OutFilePath)) {
    throw std::runtime_error("VcgGen Error: File already exists: " +
                             OutFilePath);
  }

  VcgFileStream.open(OutFilePath);
  writeGraph();
  VcgFileStream.close();
}

void VcgGen::writeGraph() {
  VcgFileStream << "graph: {" << std::endl;
  VcgFileStream << "title: "
                << "\"" << InputModule->getIdentifier() << "\"" << std::endl;

  writeProperties();

  for (auto &F : InputModule->functions()) {
    writeFunction(F.get());
  }

  VcgFileStream << "}";
}

void VcgGen::writeFunction(Function *F) {
  const std::string FunctionName = F->name();
  for (auto &BB : F->basicBlocks()) {
    writeBasicBlock(BB.get(), FunctionName);
  }
}

void VcgGen::writeBasicBlock(BasicBlock *BB, const std::string &Title) {
  VcgFileStream << "node: {" << std::endl;

  VcgFileStream << "title: "
                << "\"" << BB->toString() << "\"" << std::endl;
  VcgFileStream << "label: \""
                << "[" << Title << "]" << BB->toString() << std::endl;

  // Note: Assumes terminating instruction is in Instructions vector

  for (auto &Instr : *BB) {
    VcgFileStream << Instr->toString() << std::endl;
  }

  VcgFileStream << "\"" << std::endl;
  VcgFileStream << "}" << std::endl;

  for (auto &Next : BB->terminator()->followingBlocks()) {
    writeEdge(BB, Next);
  }
}

void VcgGen::writeEdge(BasicBlock *Source, BasicBlock *Destination) {
  VcgFileStream << "edge: {" << std::endl;

  VcgFileStream << "sourcename: "
                << "\"" << Source->toString() << "\"" << std::endl;
  VcgFileStream << "targetname: "
                << "\"" << Destination->toString() << "\"" << std::endl;
  VcgFileStream << "color: "
                << "blue" << std::endl;
  VcgFileStream << "}" << std::endl;
}

void VcgGen::writeProperties() {
  VcgFileStream << "layoutalgorithm: dfs" << std::endl;
  VcgFileStream << "manhattan_edges: yes" << std::endl;
}
