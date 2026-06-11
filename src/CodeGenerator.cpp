// MPL/src/CodeGenerator.cpp

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

#include "CodeGenerator.hpp"

static const char* regName(Reg r, uint32_t size = 8){
    static const char* n8 [16] = {
        "al","bl","cl","dl","sil","dil","spl","bpl",
        "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"
    };
    static const char* n16[16] = {
        "ax","bx","cx","dx","si","di","sp","bp",
        "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"
    };
    static const char* n32[16] = {
        "eax","ebx","ecx","edx","esi","edi","esp","ebp",
        "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"
    };
    static const char* n64[16] = {
        "rax","rbx","rcx","rdx","rsi","rdi","rsp","rbp",
        "r8","r9","r10","r11","r12","r13","r14","r15"
    };
    static const char* xmm[16] = {
        "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
        "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15"
    };
    uint8_t i = (uint8_t)r;
    if(isXmm(r)) return xmm[i - 16];
    switch(size){
        case 1:  return n8[i];
        case 2:  return n16[i];
        case 4:  return n32[i];
        default: return n64[i];
    }
}

static std::string R(Reg r, uint32_t size = 8){ return regName(r, size); }

static const char* sizeWord(uint32_t s){
    switch(s){
        case 1:  return "byte";
        case 2:  return "word";
        case 4:  return "dword";
        default: return "qword";
    }
}

static const char* intCC(BinaryOp op){
    switch(op){
        case BinaryOp::Less: return "setl";   case BinaryOp::Greater:      return "setg";
        case BinaryOp::LessEqual: return "setle"; case BinaryOp::GreaterEqual: return "setge";
        case BinaryOp::Equal: return "sete";  case BinaryOp::NotEqual:     return "setne";
        default: return "";
    }
}

static const char* sseCC(BinaryOp op){
    switch(op){
        case BinaryOp::Less: return "setb";   case BinaryOp::Greater:      return "seta";
        case BinaryOp::LessEqual: return "setbe"; case BinaryOp::GreaterEqual: return "setae";
        case BinaryOp::Equal: return "sete";  case BinaryOp::NotEqual:     return "setne";
        default: return "";
    }
}




inline uint32_t sizeOf(Type* t){
    if(auto b = dynamic_cast<BuiltinType*>(t)){
        switch(b->type){
            case BuiltinTypes::Int:   return 4;
            case BuiltinTypes::Float: return 8;
            case BuiltinTypes::Char:  return 1;
            case BuiltinTypes::Void:  return 0;
            case BuiltinTypes::Bool:  return 1;
            case BuiltinTypes::Custom: return 0; // TODO: struct size
        }
    }
    if(dynamic_cast<PointerType*>(t)) return 8;
    if(auto a = dynamic_cast<ArrayType*>(t)) return a->size * sizeOf(a->elemType.get());
    return 0;
}

inline uint32_t alignOf(Type* t){
    if(auto a = dynamic_cast<ArrayType*>(t)) return alignOf(a->elemType.get());
    return sizeOf(t) ? sizeOf(t) : 1; 
}

static RegClass regClassOf(Type* t){
      if(auto b = dynamic_cast<BuiltinType*>(t); b && b->type == BuiltinTypes::Float)
          return RegClass::Sse;
      return RegClass::Int;   // int, char, pointers
  }   


void CodeGenerator::generate(TranslationUnit& unit){
    unit.accept(*this);

    if(needRt){
        // write(2, msg, len); exit(1) — raw syscalls, works without libc
        textBuf += "rt_error:\n";
        textBuf += "\tmov rdx, rsi\n";
        textBuf += "\tmov rsi, rdi\n";
        textBuf += "\tmov rdi, 2\n";
        textBuf += "\tmov rax, 1\n";
        textBuf += "\tsyscall\n";
        textBuf += "\tmov rdi, 1\n";
        textBuf += "\tmov rax, 60\n";
        textBuf += "\tsyscall\n";
    }

    if(!dataBuf.empty()) output << "section .data\n" << dataBuf << '\n';
    if(!rodataBuf.empty()) output << "section .rodata\n" << rodataBuf << '\n';
    if(!bssBuf.empty()) output << "section .bss\n" << bssBuf << '\n';
    output << "section .text\n" << textBuf;
}

Reg CodeGenerator::alloc(RegClass cls){
    auto& pool   = (cls == RegClass::Sse) ? current.freeXmm : current.freeRegs;
    auto& inUse  = (cls == RegClass::Sse) ? current.inUseXmm : current.inUse;
    Reg r = pool.back(); pool.pop_back(); inUse.push_back(r);
    return r;
}

void CodeGenerator::freeReg(Reg r){
    bool x = isXmm(r);
    auto& pool  = x ? current.freeXmm : current.freeRegs;
    auto& inUse = x ? current.inUseXmm : current.inUse; 
    inUse.erase(std::remove(inUse.begin(), inUse.end(), r), inUse.end());
    pool.push_back(r);
}

VarInfo* CodeGenerator::findVar(std::string_view name){
    for(auto it = current.vars.rbegin(); it != current.vars.rend(); ++it){
        if(it->name == name) return &*it;
    }
    return nullptr;
}

Reg CodeGenerator::emitRValue(Expr& e){
    if(dynamic_cast<ArrayType*>(e.resultType.get())) return emitBaseAddr(e); // decay
    e.accept(*this);
    return resultReg;
}

LValue CodeGenerator::emitLValue(Expr& e){
    if(auto* id = dynamic_cast<Identifier*>(&e)){
        VarInfo* v = findVar(id->name);
        return LValue{memOf(v->rbpOffset), Reg::rax, false};
    }
    if(auto* u = dynamic_cast<UnaryExpr*>(&e); u && u->op == UnaryOp::Deref){
        Reg r = emitRValue(*u->child);
        return LValue{"[" + std::string(regName(r)) + "]", r, true};
    }
    if(auto* ix = dynamic_cast<IndexExpr*>(&e)){
        Reg base = emitBaseAddr(*ix->arr);
        Reg idx  = emitRValue(*ix->index);
        if(auto* at = dynamic_cast<ArrayType*>(ix->arr->resultType.get())){
            // unsigned compare catches negative index too, pointers stay unchecked
            current.body += "\tcmp " + R(idx) + ", " + std::to_string(at->size) + "\n";
            emitRtCheck("jb", "index out of bounds", ix->offset);
        }
        uint32_t esz = sizeOf(ix->resultType.get());
        if(esz > 1) current.body += "\timul " + R(idx) + ", " + std::to_string(esz) + "\n";
        current.body += "\tadd " + R(base) + ", " + R(idx) + "\n";
        freeReg(idx);
        return LValue{"[" + R(base) + "]", base, true};
    }
    return {};
}

//----------------------------------------
// size only at memory boundary, in regs everything is 64-bit

void CodeGenerator::emitLoad(Reg dst, const std::string& mem, Type* t){
    if(regClassOf(t) == RegClass::Sse){
        current.body += "\tmovsd " + R(dst) + ", " + mem + "\n";
        return;
    }
    uint32_t s = sizeOf(t);
    if(s == 8) current.body += "\tmov " + R(dst) + ", " + mem + "\n";
    else current.body += "\tmovsx " + R(dst) + ", " + sizeWord(s) + " " + mem + "\n";
}

void CodeGenerator::emitStore(const std::string& mem, Reg src, Type* t){
    if(regClassOf(t) == RegClass::Sse)
        current.body += "\tmovsd " + mem + ", " + R(src) + "\n";
    else
        current.body += "\tmov " + mem + ", " + R(src, sizeOf(t)) + "\n";
}

Reg CodeGenerator::loadLValue(const LValue& lv, Type* t){
    if(regClassOf(t) == RegClass::Sse){
        Reg x = alloc(RegClass::Sse);
        emitLoad(x, lv.mem, t);
        if(lv.ownsReg) freeReg(lv.reg);
        return x;
    }
    Reg dst = lv.ownsReg ? lv.reg : alloc(RegClass::Int);
    emitLoad(dst, lv.mem, t);
    return dst;
}

//----------------------------------------
// runtime checks: caller emits cmp/test, jccOk jumps over the death path
// rdi/rsi clobbered only when dying, so live regs are safe

void CodeGenerator::emitRtCheck(const std::string& jccOk, std::string_view msg, std::size_t off){
    std::string text = "runtime error: " + std::string(msg)
        + " at line " + std::to_string(smap.resolve(off, buffer).line);
    std::string lbl = "rtmsg" + std::to_string(labelId++);
    rodataBuf += lbl + ": db \"" + text + "\", 10\n";

    std::string ok = newLabel("rtok");
    current.body += "\t" + jccOk + " " + ok + "\n";
    current.body += "\tlea rdi, [rel " + lbl + "]\n";
    current.body += "\tmov rsi, " + std::to_string(text.size() + 1) + "\n";
    current.body += "\tcall rt_error\n";
    current.body += ok + ":\n";
    needRt = true;
}

void CodeGenerator::visit(TranslationUnit& node){
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
}

void CodeGenerator::visit(VarDecl& node){

}

void CodeGenerator::visit(StructDecl& node){

}

static void initFreeRegs(FuncInfo& f){
      f.freeRegs = {
          Reg::rbx, Reg::r12, Reg::r13, Reg::r14, Reg::r15,
          Reg::rcx, Reg::rsi, Reg::rdi, Reg::r8, Reg::r9, Reg::r10, Reg::r11,
      };
      f.freeXmm = {
          Reg::xmm0, Reg::xmm1, Reg::xmm2, Reg::xmm3,
          Reg::xmm4, Reg::xmm5, Reg::xmm6, Reg::xmm7,
          Reg::xmm8, Reg::xmm9, Reg::xmm10, Reg::xmm11,
          Reg::xmm12, Reg::xmm13, Reg::xmm14, Reg::xmm15,
      };
  }


static const Reg argRegs[6] = {Reg::rdi, Reg::rsi, Reg::rdx, Reg::rcx, Reg::r8, Reg::r9};


static void assignParamOffsets(FuncDecl& node, FuncInfo& f){
    int32_t stackArg = 16;
    for(std::size_t i = 0; i < node.params.size(); ++i){
        VarDecl* p = node.params[i].get();
        uint32_t s = sizeOf(p->type.get());
        uint32_t a = alignOf(p->type.get());
        VarInfo vi{ p->name, s, a, Storage::Stack};
        if(i < 6){
            f.frameSize = (f.frameSize + 8 + 7) & ~7u;
            vi.rbpOffset = -(int32_t)f.frameSize;
            f.body += "\tmov [rbp" + std::to_string(vi.rbpOffset) + "], " + regName(argRegs[i]) + "\n";
        } else {
            vi.rbpOffset = stackArg;
            stackArg += 8;
        }
        f.vars.push_back(vi);
    }
}


void CodeGenerator::emitFunction(FuncDecl& node){
    uint32_t alignedN = (current.frameSize + 15) & ~15u;
    textBuf += "global ";  textBuf += node.name;  textBuf += '\n';

    textBuf += node.name;  textBuf += ":\n";

    textBuf += "\tpush rbp\n";
    textBuf += "\tmov rbp, rsp\n";
    if(alignedN > 0) textBuf += "\tsub rsp, " + std::to_string(alignedN) + '\n';

    textBuf += current.body;

    textBuf += ".Lreturn_"; textBuf += node.name;  textBuf += ":\n";
    textBuf += "\tleave\n";
    textBuf += "\tret\n\n";
}

void CodeGenerator::visit(FuncDecl& node){
    if(!node.body){
        textBuf += "extern " + std::string(node.name) + "\n";   // ← NEW
        return; 
    }

    current = FuncInfo{};
    initFreeRegs(current);
    current.name = node.name;
    assignParamOffsets(node, current);

    node.body->accept(*this);

    emitFunction(node);
}


void CodeGenerator::visit(BlockStmt& node){
    std::size_t mark = current.vars.size();
    for(auto& stmt : node.statements){
        stmt->accept(*this);
    }
    current.vars.resize(mark);
}

void CodeGenerator::visit(ExprStmt& node){
    if(node.expr){
        Reg r = emitRValue(*node.expr);
        freeReg(r);
    }   
}

void CodeGenerator::visit(IfStmt& node){
    std::string endL  = newLabel("endif");
    std::string elseL = node.elsePart ? newLabel("else") : endL;
    Reg c = emitRValue(*node.cond);
    current.body += "\tcmp " + std::string(regName(c)) + ", 0\n";
    freeReg(c);
    current.body += "\tje " + elseL + "\n";
    node.thenPart->accept(*this);
    if(node.elsePart){
        current.body += "\tjmp " + endL + "\n" + elseL + ":\n";
        node.elsePart->accept(*this);
    }   
    current.body += endL + ":\n";
}

void CodeGenerator::visit(WhileStmt& node){
    std::string condL = newLabel("while"), endL = newLabel("endwhile");
    loopStack.push_back({condL, endL});
    current.body += condL + ":\n";
    Reg c = emitRValue(*node.cond);
    current.body += "\tcmp " + std::string(regName(c)) + ", 0\n";
    freeReg(c);
    current.body += "\tje " + endL + "\n";
    node.body->accept(*this);
    current.body += "\tjmp " + condL + "\n" + endL + ":\n";
    loopStack.pop_back();

}

void CodeGenerator::visit(ForStmt& node){
    std::string condL = newLabel("for"), contL = newLabel("forcont"), endL = newLabel("endfor");
    if(node.initDecl){ if(auto vd = dynamic_cast<VarDecl*>(node.initDecl.get())) emitLocalVar(*vd); }
    if(node.initStmt) node.initStmt->accept(*this);
    loopStack.push_back({contL, endL});
    current.body += condL + ":\n";
    if(node.cond){
        Reg c = emitRValue(*node.cond);
        current.body += "\tcmp " + std::string(regName(c)) + ", 0\n";
        freeReg(c);
        current.body += "\tje " + endL + "\n";
    }
    node.body->accept(*this);
    current.body += contL + ":\n";
    if(node.incr){ Reg r = emitRValue(*node.incr); freeReg(r); }
    current.body += "\tjmp " + condL + "\n" + endL + ":\n";
    loopStack.pop_back();

}

void CodeGenerator::visit(ReturnStmt& node){
if(node.value){
          Reg r = emitRValue(*node.value);
          if(r != Reg::rax)
              current.body += "\tmov rax, " + std::string(regName(r)) + "\n";
          freeReg(r);
      }
      current.body += "\tjmp .Lreturn_" + current.name + "\n";

}

void CodeGenerator::visit(BreakStmt& node){
    current.body += "\tjmp " + loopStack.back().second + "\n";
}

void CodeGenerator::visit(ContinueStmt& node){
    current.body += "\tjmp " + loopStack.back().first  + "\n";
}

void CodeGenerator::emitLocalVar(VarDecl& vd){
    uint32_t s = sizeOf(vd.type.get());
    uint32_t a = alignOf(vd.type.get());
    current.frameSize = (current.frameSize + s + (a-1)) & ~(a-1);
    int32_t off = -(int32_t)current.frameSize; 
    current.vars.push_back(VarInfo{ vd.name, s, a, Storage::Stack, {} });
    current.vars.back().rbpOffset = off;
    if(vd.init){
        Reg r = emitRValue(*vd.init);
        emitStore(memOf(off), r, vd.type.get());
        freeReg(r);
    }
}


void CodeGenerator::visit(DeclStmt& node){
    auto* vd = dynamic_cast<VarDecl*>(node.decl.get());
    if(vd){ emitLocalVar(*vd);}
}

inline bool isArithmetic(BinaryOp op){ return op >= BinaryOp::Add  && op <= BinaryOp::Mod; }
inline bool isComparison(BinaryOp op){ return op >= BinaryOp::Less && op <= BinaryOp::GreaterEqual; }
inline bool isLogical(BinaryOp op)   { return op == BinaryOp::And  || op == BinaryOp::Or; }
inline bool isAssign(BinaryOp op)    { return op >= BinaryOp::Assign; }




void CodeGenerator::visit(BinaryExpr& node){
    if(isAssign(node.op)){
        LValue lv = emitLValue(*node.left);
        Reg r = emitRValue(*node.right);
        Type* lt = node.left->resultType.get();
        if(node.op == BinaryOp::Assign){
            emitStore(lv.mem, r, lt);
        }else{
            if(regClassOf(lt) == RegClass::Sse){
                Reg t = alloc(RegClass::Sse);
                current.body += "\tmovsd " + R(t) + ", " + lv.mem + "\n";
                current.body += "\t" + std::string(node.op==BinaryOp::AddAssign ? "addsd":"subsd")
                    + " " + R(t) + ", " + R(r) + "\n";
                current.body += "\tmovsd " + lv.mem + ", " + R(t) + "\n";
                freeReg(t);
            }else{
                current.body += "\t" + std::string(node.op==BinaryOp::AddAssign ? "add":"sub")
                    + " " + lv.mem + ", " + R(r, sizeOf(lt)) + "\n";
            }
        }
        if(lv.ownsReg) freeReg(lv.reg);
        resultReg = r;
    }
    else if(isLogical(node.op)){
        std::string endL = newLabel("logic");
        Reg res = emitRValue(*node.left);
        current.body += "\tcmp " + R(res) + ", 0\n";
        current.body += (node.op==BinaryOp::And ? "\tje " : "\tjne ") + endL + "\n";
        Reg r = emitRValue(*node.right);
        current.body += "\tmov " + R(res) + ", " + R(r) + "\n";
        freeReg(r);
        current.body += endL + ":\n";
        current.body += "\tcmp " + R(res) + ", 0\n\tsetne al\n\tmovzx " + R(res) + ", al\n";
        resultReg = res;
        return;
    }
    else{
        RegClass cls = regClassOf(node.left->resultType.get());
        Reg l = emitRValue(*node.left);
        Reg r = emitRValue(*node.right);

        if(isComparison(node.op)){
            if(cls == RegClass::Sse){
                current.body += "\tucomisd " + R(l) + ", " + R(r) + "\n";
                Reg res = alloc(RegClass::Int);
                current.body += "\t" + std::string(sseCC(node.op)) + " al\n";
                current.body += "\tmovzx " + R(res) + ", al\n";
                freeReg(l); freeReg(r);
                resultReg = res;
            }else{
                current.body += "\tcmp " + R(l) + ", " + R(r) + "\n";
                current.body += "\t" + std::string(intCC(node.op)) + " al\n";
                current.body += "\tmovzx " + std::string(regName(l)) + ", al\n";
                freeReg(r);
                resultReg = l;
            }
            return;
        }
        if(cls == RegClass::Sse){
            const char* m = node.op==BinaryOp::Add ? "addsd" : node.op==BinaryOp::Sub ? "subsd"
                : node.op==BinaryOp::Mul ? "mulsd" : "divsd";   
            current.body += "\t" + std::string(m) + " " + R(l) + ", " + R(r) + "\n";
        }else{
            switch(node.op){
                case BinaryOp::Add: 
                    if(auto pt = dynamic_cast<PointerType*>(node.left->resultType.get())){
                        uint32_t esz = sizeOf(pt->base.get());
                        if(esz > 1) current.body += "\timul " + R(r) + ", " + std::to_string(esz) + "\n";
                    }
                    current.body += "\tadd "  + R(l) + ", " + R(r) + "\n"; break;
                case BinaryOp::Sub:
                    if(auto pt = dynamic_cast<PointerType*>(node.left->resultType.get())){
                        uint32_t esz = sizeOf(pt->base.get());
                        if(esz > 1) current.body += "\timul " + R(r) + ", " + std::to_string(esz) + "\n";
                    }
                    current.body += "\tsub "  + R(l) + ", " + R(r) + "\n"; break;
                case BinaryOp::Mul: current.body += "\timul " + R(l) + ", " + R(r) + "\n"; break;
                case BinaryOp::Div: case BinaryOp::Mod:
                    current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
                    emitRtCheck("jnz", "division by zero", node.offset);
                    current.body += "\tmov rax, " + R(l) + "\n\tcqo\n\tidiv " + R(r) + "\n";
                    current.body += "\tmov " + R(l) + ", " + (node.op==BinaryOp::Div ? "rax" : "rdx") + "\n";
                    break;
                default: break;
            }
        }
        freeReg(r);
        resultReg = l;
    }
}

void CodeGenerator::visit(UnaryExpr& node){
    if(node.op == UnaryOp::Deref){
        resultReg = loadLValue(emitLValue(node), node.resultType.get());
        return;
    }
    if(node.op == UnaryOp::AddressOf){
        LValue lv = emitLValue(*node.child);
        if(lv.ownsReg){resultReg = lv.reg;}
        else{Reg r = alloc();
                current.body += "\tlea " + R(r) + ", " + lv.mem + "\n";
                resultReg = r;}
        return;
    }
    if(node.op == UnaryOp::Neg){
        Reg r = emitRValue(*node.child);
        if(regClassOf(node.resultType.get()) == RegClass::Sse){
            Reg z = alloc(RegClass::Sse);
            current.body += "\txorpd " + R(z) + ", " + R(z) + "\n";
            current.body += "\tsubsd " + R(z) + ", " + R(r) + "\n"; // 0.0 - x
            freeReg(r);
            resultReg = z;
        }else{
            current.body += "\tneg " + R(r) + "\n";
            resultReg = r;
        }
        return;
    }
    if(node.op == UnaryOp::Not){
        Reg r = emitRValue(*node.child);
        current.body += "\tcmp " + R(r) + ", 0\n\tsete al\n\tmovzx " + R(r) + ", al\n";
        resultReg = r;
        return;
    }
    if(node.op == UnaryOp::BitNot){
        Reg r = emitRValue(*node.child);
        current.body += "\tnot " + R(r) + "\n";
        resultReg = r;
        return;
    }
    // TODO PreInc PreDec PostInc PostDec
}

void CodeGenerator::visit(CallExpr& node){
    static const Reg argReg[6] = {Reg::rdi, Reg::rsi, Reg::rdx, Reg::rcx, Reg::r8, Reg::r9};
    auto* id = dynamic_cast<Identifier*>(node.func.get());
    std::string name = std::string(id->name);
    std::size_t n = node.param.size();


    std::vector<Reg> spill = current.inUse;                
    for(Reg r : spill) current.body += "\tpush " + std::string(regName(r)) + "\n";
    bool pad = (spill.size() % 2) != 0;                     
    if(pad) current.body += "\tsub rsp, 8\n";


    for(std::size_t i = 0; i < n; ++i){
        Reg r = emitRValue(*node.param[i]);
        current.body += "\tpush " + std::string(regName(r)) + "\n";
        freeReg(r);
    }
    for(std::size_t i = n; i-- > 0; )
        current.body += "\tpop " + std::string(regName(argReg[i])) + "\n";

    current.body += "\txor eax, eax\n";
    current.body += "\tcall " + name + "\n";
    current.hasCall = true;

    if(pad) current.body += "\tadd rsp, 8\n";
    for(std::size_t i = spill.size(); i-- > 0; )
        current.body += "\tpop " + std::string(regName(spill[i])) + "\n";

    Reg res = alloc(regClassOf(node.resultType.get()));
    current.body += "\tmov " + std::string(regName(res)) + ", rax\n";
    resultReg = res;

}

void CodeGenerator::visit(CastExpr& node){
    Reg src = emitRValue(*node.expr);
    Type* to = node.target.get();
    RegClass fromCls = regClassOf(node.expr->resultType.get());
    RegClass toCls   = regClassOf(to);

    if(fromCls == toCls){
        if(toCls == RegClass::Int){
            uint32_t s = sizeOf(to);
            if(auto b = dynamic_cast<BuiltinType*>(to); b && b->type == BuiltinTypes::Bool)
                current.body += "\tcmp " + R(src) + ", 0\n\tsetne al\n\tmovzx " + R(src) + ", al\n";
            else if(s < 8)
                current.body += "\tmovsx " + R(src) + ", " + R(src, s) + "\n";
        }
        resultReg = src;
        return;
    }
    Reg dst = alloc(toCls);
    if(toCls == RegClass::Sse)
        current.body += "\tcvtsi2sd " + R(dst) + ", " + R(src) + "\n";
    else
        current.body += "\tcvttsd2si " + R(dst) + ", " + R(src) + "\n";
    freeReg(src);
    resultReg = dst;
}

Reg CodeGenerator::emitBaseAddr(Expr& arr){
    if(dynamic_cast<ArrayType*>(arr.resultType.get())){
        LValue lv = emitLValue(arr);
        Reg r = alloc();
        current.body += "\tlea " + R(r) + ", " + lv.mem + "\n";
        if(lv.ownsReg) freeReg(lv.reg);
        return r;
    }
    return emitRValue(arr);
}


void CodeGenerator::visit(IndexExpr& node){
    resultReg = loadLValue(emitLValue(node), node.resultType.get());
}

void CodeGenerator::visit(AccessExpr& node){

}

void CodeGenerator::visit(IntLiteral& node){
    Reg r = alloc(regClassOf(node.resultType.get()));
    current.body += "\tmov " + std::string(regName(r)) + ", " + std::to_string(node.value) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(FloatLiteral& node){
    uint64_t bits;
    std::memcpy(&bits, &node.value, sizeof bits);
    char buf[24];
    std::snprintf(buf, sizeof buf, "0x%016llX", (unsigned long long)bits);
    std::string lbl = "Lflt" + std::to_string(labelId++);
    rodataBuf += lbl + ": dq " + buf + " ; " + std::to_string(node.value) + "\n";
    Reg r = alloc(RegClass::Sse);
    current.body += "\tmovsd " + R(r) + ", [rel " + lbl + "]\n";
    resultReg = r;
}

void CodeGenerator::visit(CharLiteral& node){
    Reg r = alloc(RegClass::Int);
    current.body += "\tmov " + R(r) + ", " + std::to_string((int)node.value) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(BoolLiteral& node){
    Reg r = alloc(regClassOf(node.resultType.get()));
    current.body += "\tmov " + std::string(regName(r)) + ", " + std::to_string(node.value ? 1 : 0) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(StringLiteral& node){
    std::string lbl = "Lstr" + std::to_string(labelId++);
    std::string s(node.value);
    rodataBuf += lbl + ": db ";
    bool first = true;
    auto put = [&](int b){ rodataBuf += (first?"":", ") + std::to_string(b); first = false; };
    for(std::size_t i = 0; i < s.size(); ++i){
        if(s[i]=='\\' && i+1<s.size()){
            char n = s[++i];
            put(n=='n'?10 : n=='t'?9 : n=='r'?13 : n=='0'?0 : n);
        } else put((unsigned char)s[i]);
    }   
    put(0);
    rodataBuf += "\n";                                        
    Reg r = alloc();
    current.body += "\tlea " + std::string(regName(r)) + ", [abs " + lbl + "]\n";
    resultReg = r;
}

void CodeGenerator::visit(Identifier& node){
    resultReg = loadLValue(emitLValue(node), node.resultType.get());
}

void CodeGenerator::visit(BuiltinType&){

}

void CodeGenerator::visit(PointerType&){

}

void CodeGenerator::visit(FuncType&){

}

void CodeGenerator::visit(ArrayType&){

}

