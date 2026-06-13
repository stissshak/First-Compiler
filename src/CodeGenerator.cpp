// MPL/src/CodeGenerator.cpp

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "CodeGenerator.hpp"

static const char* regName(Reg r, uint32_t size = 8) {
    static const char* n8[16] = {"al",  "bl",  "cl",   "dl",   "sil",  "dil",  "spl",  "bpl",
                                 "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
    static const char* n16[16] = {"ax",  "bx",  "cx",   "dx",   "si",   "di",   "sp",   "bp",
                                  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
    static const char* n32[16] = {"eax", "ebx", "ecx",  "edx",  "esi",  "edi",  "esp",  "ebp",
                                  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
    static const char* n64[16] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
                                  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    static const char* xmm[16] = {"xmm0",  "xmm1",  "xmm2",  "xmm3", "xmm4",  "xmm5",
                                  "xmm6",  "xmm7",  "xmm8",  "xmm9", "xmm10", "xmm11",
                                  "xmm12", "xmm13", "xmm14", "xmm15"};
    uint8_t i = (uint8_t)r;
    if (isXmm(r))
        return xmm[i - 16];
    switch (size) {
    case 1:
        return n8[i];
    case 2:
        return n16[i];
    case 4:
        return n32[i];
    default:
        return n64[i];
    }
}

static std::string R(Reg r, uint32_t size = 8) { return regName(r, size); }

static const char* sizeWord(uint32_t s) {
    switch (s) {
    case 1:
        return "byte";
    case 2:
        return "word";
    case 4:
        return "dword";
    default:
        return "qword";
    }
}

static const char* intCC(BinaryOp op) {
    switch (op) {
    case BinaryOp::Less:
        return "setl";
    case BinaryOp::Greater:
        return "setg";
    case BinaryOp::LessEqual:
        return "setle";
    case BinaryOp::GreaterEqual:
        return "setge";
    case BinaryOp::Equal:
        return "sete";
    case BinaryOp::NotEqual:
        return "setne";
    default:
        return "";
    }
}

static const char* uintCC(BinaryOp op) {
    switch (op) {
    case BinaryOp::Less:
        return "setb";
    case BinaryOp::Greater:
        return "seta";
    case BinaryOp::LessEqual:
        return "setbe";
    case BinaryOp::GreaterEqual:
        return "setae";
    case BinaryOp::Equal:
        return "sete";
    case BinaryOp::NotEqual:
        return "setne";
    default:
        return "";
    }
}

static bool isUnsigned(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    return b && b->type == BuiltinTypes::UInt;
}

//----------------------------------------

struct StructField {
    std::string_view name;
    uint32_t offset;
    Type* type;
};

struct StructLayout {
    uint32_t size = 0, align = 1;
    std::vector<StructField> fields;
};
static std::unordered_map<std::string_view, StructLayout> structLayouts;

static StructLayout* layoutOf(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    if (!b || b->type != BuiltinTypes::Custom)
        return nullptr;
    auto it = structLayouts.find(b->name);
    return it == structLayouts.end() ? nullptr : &it->second;
}

inline uint32_t sizeOf(Type* t) {
    if (auto b = dynamic_cast<BuiltinType*>(t)) {
        switch (b->type) {
        case BuiltinTypes::Int:
            return 4;
        case BuiltinTypes::UInt:
            return 4;
        case BuiltinTypes::Short:
            return 2;
        case BuiltinTypes::Long:
            return 8;
        case BuiltinTypes::Float:
            return 8;
        case BuiltinTypes::Char:
            return 1;
        case BuiltinTypes::Void:
            return 0;
        case BuiltinTypes::Bool:
            return 1;
        case BuiltinTypes::Byte:
            return 1;
        case BuiltinTypes::Custom: {
            auto l = layoutOf(t);
            return l ? l->size : 0;
        }
        }
    }
    if (dynamic_cast<PointerType*>(t))
        return 8;
    if (dynamic_cast<FuncType*>(t))
        return 8; // func pointer
    if (auto a = dynamic_cast<ArrayType*>(t))
        return a->size * sizeOf(a->elemType.get());
    return 0;
}

inline uint32_t alignOf(Type* t) {
    if (auto a = dynamic_cast<ArrayType*>(t))
        return alignOf(a->elemType.get());
    if (auto l = layoutOf(t))
        return l->align;
    return sizeOf(t) ? sizeOf(t) : 1;
}

static RegClass regClassOf(Type* t) {
    if (auto b = dynamic_cast<BuiltinType*>(t); b && b->type == BuiltinTypes::Float)
        return RegClass::Sse;
    return RegClass::Int; // int, char, pointers
}

void CodeGenerator::generate(TranslationUnit& unit) {
    unit.accept(*this);

    if (needRt) {
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

    std::string ext;
    for (auto& name : usedExterns)
        if (!funcs.count(name))
            ext += "extern " + name + "\n";

    if (!dataBuf.empty())
        output << "section .data\n" << dataBuf << '\n';
    if (!rodataBuf.empty())
        output << "section .rodata\n" << rodataBuf << '\n';
    if (!bssBuf.empty())
        output << "section .bss\n" << bssBuf << '\n';
    output << "section .text\n" << ext << textBuf;
}

Reg CodeGenerator::alloc(RegClass cls) {
    auto& pool = (cls == RegClass::Sse) ? current.freeXmm : current.freeRegs;
    auto& inUse = (cls == RegClass::Sse) ? current.inUseXmm : current.inUse;
    Reg r = pool.back();
    pool.pop_back();
    inUse.push_back(r);
    return r;
}

void CodeGenerator::freeReg(Reg r) {
    bool x = isXmm(r);
    auto& pool = x ? current.freeXmm : current.freeRegs;
    auto& inUse = x ? current.inUseXmm : current.inUse;
    inUse.erase(std::remove(inUse.begin(), inUse.end(), r), inUse.end());
    pool.push_back(r);
}

VarInfo* CodeGenerator::findVar(std::string_view name) {
    for (auto it = current.vars.rbegin(); it != current.vars.rend(); ++it) {
        if (it->name == name)
            return &*it;
    }
    return nullptr;
}

Reg CodeGenerator::emitRValue(Expr& e) {
    if (dynamic_cast<ArrayType*>(e.resultType.get()))
        return emitBaseAddr(e); // decay
    e.accept(*this);
    return resultReg;
}

LValue CodeGenerator::emitLValue(Expr& e) {
    if (auto* id = dynamic_cast<Identifier*>(&e)) {
        VarInfo* v = findVar(id->name);
        if (!v)
            return LValue{"[rel " + std::string(id->name) + "]", Reg::rax, false}; // global
        return LValue{memOf(v->rbpOffset), Reg::rax, false};
    }
    if (auto* u = dynamic_cast<UnaryExpr*>(&e); u && u->op == UnaryOp::Deref) {
        Reg r = emitRValue(*u->child);
        current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
        emitRtCheck("jnz", "null pointer dereference", u->offset);
        return LValue{"[" + std::string(regName(r)) + "]", r, true};
    }
    if (auto* ax = dynamic_cast<AccessExpr*>(&e)) {
        Reg r;
        Type* st = ax->object->resultType.get();
        if (ax->kind == AccessKind::Arrow) {
            r = emitRValue(*ax->object);
            st = static_cast<PointerType*>(st)->base.get();
            current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
            emitRtCheck("jnz", "null pointer dereference", ax->offset);
        } else {
            r = lvalueAddr(*ax->object);
        }
        for (auto& f : layoutOf(st)->fields) {
            if (f.name != ax->field)
                continue;
            if (f.offset)
                current.body += "\tadd " + R(r) + ", " + std::to_string(f.offset) + "\n";
            break;
        }
        return LValue{"[" + R(r) + "]", r, true};
    }
    if (auto* ix = dynamic_cast<IndexExpr*>(&e)) {
        Reg base = emitBaseAddr(*ix->arr);
        Reg idx = emitRValue(*ix->index);
        if (auto* at = dynamic_cast<ArrayType*>(ix->arr->resultType.get())) {
            current.body += "\tcmp " + R(idx) + ", " + std::to_string(at->size) + "\n";
            emitRtCheck("jb", "index out of bounds", ix->offset);
        } else {
            current.body += "\ttest " + R(base) + ", " + R(base) + "\n";
            emitRtCheck("jnz", "null pointer dereference", ix->offset);
        }
        uint32_t esz = sizeOf(ix->resultType.get());
        if (esz > 1)
            current.body += "\timul " + R(idx) + ", " + std::to_string(esz) + "\n";
        current.body += "\tadd " + R(base) + ", " + R(idx) + "\n";
        freeReg(idx);
        return LValue{"[" + R(base) + "]", base, true};
    }
    return {};
}

//----------------------------------------

void CodeGenerator::emitLoad(Reg dst, const std::string& mem, Type* t) {
    if (regClassOf(t) == RegClass::Sse) {
        current.body += "\tmovsd " + R(dst) + ", " + mem + "\n";
        return;
    }
    uint32_t s = sizeOf(t);
    if (s == 8)
        current.body += "\tmov " + R(dst) + ", " + mem + "\n";
    else if (isUnsigned(t))
        current.body += "\tmov " + R(dst, 4) + ", " + mem + "\n";
    else
        current.body += "\tmovsx " + R(dst) + ", " + sizeWord(s) + " " + mem + "\n";
}

void CodeGenerator::emitStore(const std::string& mem, Reg src, Type* t) {
    if (regClassOf(t) == RegClass::Sse)
        current.body += "\tmovsd " + mem + ", " + R(src) + "\n";
    else
        current.body += "\tmov " + mem + ", " + R(src, sizeOf(t)) + "\n";
}

Reg CodeGenerator::loadLValue(const LValue& lv, Type* t) {
    if (regClassOf(t) == RegClass::Sse) {
        Reg x = alloc(RegClass::Sse);
        emitLoad(x, lv.mem, t);
        if (lv.ownsReg)
            freeReg(lv.reg);
        return x;
    }
    Reg dst = lv.ownsReg ? lv.reg : alloc(RegClass::Int);
    emitLoad(dst, lv.mem, t);
    return dst;
}

Reg CodeGenerator::lvalueAddr(Expr& e) {
    LValue lv = emitLValue(e);
    if (lv.ownsReg)
        return lv.reg;
    Reg r = alloc();
    current.body += "\tlea " + R(r) + ", " + lv.mem + "\n";
    return r;
}

static int hexVal(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// decode a string literal's escapes into raw bytes, trailing nul included
static std::vector<uint8_t> decodeStr(std::string_view s) {
    std::vector<uint8_t> out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            if (n == 'x') { // \xHH... hex escape
                int v = 0, d;
                while (i + 1 < s.size() && (d = hexVal(s[i + 1])) >= 0) {
                    v = v * 16 + d;
                    ++i;
                }
                out.push_back((uint8_t)v);
            } else
                out.push_back(n == 'n'   ? 10
                              : n == 't' ? 9
                              : n == 'r' ? 13
                              : n == '0' ? 0
                              : n == 'a' ? 7
                              : n == 'b' ? 8
                              : n == 'f' ? 12
                              : n == 'v' ? 11
                                         : (uint8_t)n); // \\ \" \' etc: literal
        } else
            out.push_back((uint8_t)s[i]);
    }
    out.push_back(0);
    return out;
}

void CodeGenerator::emitMemCopy(Reg dst, Reg src, uint32_t sz) {
    Reg t = alloc();
    for (uint32_t o = 0; o < sz;) {
        uint32_t step = sz - o >= 8 ? 8 : sz - o >= 4 ? 4 : sz - o >= 2 ? 2 : 1;
        current.body += "\tmov " + R(t, step) + ", [" + R(src) + " + " + std::to_string(o) + "]\n";
        current.body += "\tmov [" + R(dst) + " + " + std::to_string(o) + "], " + R(t, step) + "\n";
        o += step;
    }
    freeReg(t);
}

//----------------------------------------

void CodeGenerator::emitRtCheck(const std::string& jccOk, std::string_view msg, std::size_t off) {
    std::string text = "runtime error: " + std::string(msg) + " at line " +
                       std::to_string(smap.resolve(off, buffer).line);
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

void CodeGenerator::visit(TranslationUnit& node) {
    for (std::size_t i = 0; i < node.decls.size(); ++i) {
        node.decls[i]->accept(*this);
    }
}

void CodeGenerator::visit(VarDecl& node) {
    uint32_t s = sizeOf(node.type.get());
    std::string name(node.name);
    if (!node.initList.empty()) {
        if (auto at = dynamic_cast<ArrayType*>(node.type.get())) {
            uint32_t es = sizeOf(at->elemType.get());
            const char* w = es == 1 ? "db" : es == 2 ? "dw" : es == 4 ? "dd" : "dq";
            dataBuf += name + ": " + w + " ";
            for (std::size_t i = 0; i < at->size; ++i) {
                long v = 0;
                if (i < node.initList.size()) {
                    if (auto il = dynamic_cast<IntLiteral*>(node.initList[i].get()))
                        v = il->value;
                    if (auto chl = dynamic_cast<CharLiteral*>(node.initList[i].get()))
                        v = (unsigned char)chl->value;
                    if (auto bl = dynamic_cast<BoolLiteral*>(node.initList[i].get()))
                        v = bl->value;
                }
                dataBuf += std::to_string(v) + (i + 1 < at->size ? ", " : "\n");
            }
        }
        // TODO global struct literal
        return;
    }
    if (!node.init) {
        bssBuf += name + ": resb " + std::to_string(s) + "\n";
        return;
    }
    const char* word = s == 1 ? "db" : s == 2 ? "dw" : s == 4 ? "dd" : "dq";
    if (auto il = dynamic_cast<IntLiteral*>(node.init.get()))
        dataBuf += name + ": " + word + " " + std::to_string(il->value) + "\n";
    else if (auto chl = dynamic_cast<CharLiteral*>(node.init.get()))
        dataBuf += name + ": " + word + " " + std::to_string((int)chl->value) + "\n";
    else if (auto bl = dynamic_cast<BoolLiteral*>(node.init.get()))
        dataBuf += name + ": " + word + " " + std::to_string(bl->value ? 1 : 0) + "\n";
    else if (auto fl = dynamic_cast<FloatLiteral*>(node.init.get())) {
        uint64_t bits;
        std::memcpy(&bits, &fl->value, sizeof bits);
        char buf[24];
        std::snprintf(buf, sizeof buf, "0x%016llX", (unsigned long long)bits);
        dataBuf += name + ": dq " + buf + " ; " + std::to_string(fl->value) + "\n";
    }
    // TODO non-literal global init
}

void CodeGenerator::visit(StructDecl& node) {
    StructLayout l;
    for (auto& f : node.fields) {
        uint32_t s = sizeOf(f->type.get());
        uint32_t a = alignOf(f->type.get());
        l.align = std::max(l.align, a);
        l.size = (l.size + a - 1) & ~(a - 1); // pad to field align
        l.fields.push_back({f->name, l.size, f->type.get()});
        l.size += s;
    }
    l.size = (l.size + l.align - 1) & ~(l.align - 1); // tail padding
    structLayouts[node.name] = l;
}

static void initFreeRegs(FuncInfo& f) {
    f.freeRegs = {
        Reg::rbx, Reg::r12, Reg::r13, Reg::r14, Reg::r15, Reg::rsi,
        Reg::rdi, Reg::r8,  Reg::r9,  Reg::r10, Reg::r11,
    };
    f.freeXmm = {
        Reg::xmm0,  Reg::xmm1,  Reg::xmm2,  Reg::xmm3,  Reg::xmm4,  Reg::xmm5,
        Reg::xmm6,  Reg::xmm7,  Reg::xmm8,  Reg::xmm9,  Reg::xmm10, Reg::xmm11,
        Reg::xmm12, Reg::xmm13, Reg::xmm14, Reg::xmm15,
    };
}

static const Reg argRegs[6] = {Reg::rdi, Reg::rsi, Reg::rdx, Reg::rcx, Reg::r8, Reg::r9};

static void assignParamOffsets(FuncDecl& node, FuncInfo& f) {
    int32_t stackArg = 16;
    std::size_t gp = 0, sse = 0; // SysV: GP and SSE args count separately
    for (std::size_t i = 0; i < node.params.size(); ++i) {
        VarDecl* p = node.params[i].get();
        uint32_t s = sizeOf(p->type.get());
        uint32_t a = alignOf(p->type.get());
        VarInfo vi{p->name, s, a, Storage::Stack, {Section::Text}};
        bool isSse = regClassOf(p->type.get()) == RegClass::Sse;
        bool inReg = isSse ? sse < 8 : gp < 6;
        if (inReg) {
            f.frameSize = (f.frameSize + 8 + 7) & ~7u;
            vi.rbpOffset = -(int32_t)f.frameSize;
            if (isSse)
                f.body += "\tmovsd " + CodeGenerator::memOf(vi.rbpOffset) + ", xmm" +
                          std::to_string(sse++) + "\n";
            else
                f.body += "\tmov " + CodeGenerator::memOf(vi.rbpOffset) + ", " +
                          regName(argRegs[gp++]) + "\n";
        } else {
            vi.rbpOffset = stackArg;
            stackArg += 8;
        }
        f.vars.push_back(vi);
    }
}

void CodeGenerator::emitFunction(FuncDecl& node) {
    uint32_t alignedN = (current.frameSize + 15) & ~15u;
    textBuf += "global ";
    textBuf += node.name;
    textBuf += '\n';

    textBuf += node.name;
    textBuf += ":\n";

    textBuf += "\tpush rbp\n";
    textBuf += "\tmov rbp, rsp\n";
    if (alignedN > 0)
        textBuf += "\tsub rsp, " + std::to_string(alignedN) + '\n';

    textBuf += current.body;

    textBuf += ".Lreturn_";
    textBuf += node.name;
    textBuf += ":\n";
    textBuf += "\tleave\n";
    textBuf += "\tret\n\n";
}

void CodeGenerator::visit(FuncDecl& node) {
    funcs[node.name] = &node;
    if (!node.body) {
        if (node.isExtern)
            textBuf += "extern " + std::string(node.name) + "\n";
        return;
    }

    current = FuncInfo{};
    initFreeRegs(current);
    current.name = node.name;
    assignParamOffsets(node, current);

    node.body->accept(*this);

    emitFunction(node);
}

void CodeGenerator::visit(BlockStmt& node) {
    std::size_t mark = current.vars.size();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    current.vars.resize(mark);
}

void CodeGenerator::visit(ExprStmt& node) {
    if (node.expr) {
        Reg r = emitRValue(*node.expr);
        freeReg(r);
    }
}

void CodeGenerator::visit(IfStmt& node) {
    std::string endL = newLabel("endif");
    std::string elseL = node.elsePart ? newLabel("else") : endL;
    Reg c = emitRValue(*node.cond);
    current.body += "\tcmp " + std::string(regName(c)) + ", 0\n";
    freeReg(c);
    current.body += "\tje " + elseL + "\n";
    node.thenPart->accept(*this);
    if (node.elsePart) {
        current.body += "\tjmp " + endL + "\n" + elseL + ":\n";
        node.elsePart->accept(*this);
    }
    current.body += endL + ":\n";
}

void CodeGenerator::visit(WhileStmt& node) {
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

void CodeGenerator::visit(ForStmt& node) {
    std::string condL = newLabel("for"), contL = newLabel("forcont"), endL = newLabel("endfor");
    if (node.initDecl) {
        if (auto vd = dynamic_cast<VarDecl*>(node.initDecl.get()))
            emitLocalVar(*vd);
    }
    if (node.initStmt)
        node.initStmt->accept(*this);
    loopStack.push_back({contL, endL});
    current.body += condL + ":\n";
    if (node.cond) {
        Reg c = emitRValue(*node.cond);
        current.body += "\tcmp " + std::string(regName(c)) + ", 0\n";
        freeReg(c);
        current.body += "\tje " + endL + "\n";
    }
    node.body->accept(*this);
    current.body += contL + ":\n";
    if (node.incr) {
        Reg r = emitRValue(*node.incr);
        freeReg(r);
    }
    current.body += "\tjmp " + condL + "\n" + endL + ":\n";
    loopStack.pop_back();
}

void CodeGenerator::visit(ReturnStmt& node) {
    if (node.value) {
        Reg r = emitRValue(*node.value);
        if (isXmm(r)) { // float returns in xmm0
            if (r != Reg::xmm0)
                current.body += "\tmovsd xmm0, " + std::string(regName(r)) + "\n";
        } else if (r != Reg::rax) {
            current.body += "\tmov rax, " + std::string(regName(r)) + "\n";
        }
        freeReg(r);
    }
    current.body += "\tjmp .Lreturn_" + current.name + "\n";
}

void CodeGenerator::visit(BreakStmt&) { current.body += "\tjmp " + loopStack.back().second + "\n"; }

void CodeGenerator::visit(ContinueStmt&) {
    current.body += "\tjmp " + loopStack.back().first + "\n";
}

void CodeGenerator::emitLocalVar(VarDecl& vd) {
    uint32_t s = sizeOf(vd.type.get());
    uint32_t a = alignOf(vd.type.get());
    current.frameSize = (current.frameSize + s + (a - 1)) & ~(a - 1);
    int32_t off = -(int32_t)current.frameSize;
    current.vars.push_back(VarInfo{vd.name, s, a, Storage::Stack, {}});
    current.vars.back().rbpOffset = off;
    if (!vd.initList.empty()) {
        if (auto at = dynamic_cast<ArrayType*>(vd.type.get())) {
            uint32_t esz = sizeOf(at->elemType.get());
            for (std::size_t i = 0; i < vd.initList.size(); ++i) {
                Reg r = emitRValue(*vd.initList[i]);
                emitStore(memOf(off + (int32_t)(i * esz)), r, at->elemType.get());
                freeReg(r);
            }
        } else if (auto l = layoutOf(vd.type.get())) {
            for (std::size_t i = 0; i < vd.initList.size(); ++i) {
                Reg r = emitRValue(*vd.initList[i]);
                emitStore(memOf(off + (int32_t)l->fields[i].offset), r, l->fields[i].type);
                freeReg(r);
            }
        }
        return;
    }
    if (vd.init) {
        // char[] = "literal": store decoded bytes, zero-fill the rest
        if (dynamic_cast<ArrayType*>(vd.type.get())) {
            if (auto sl = dynamic_cast<StringLiteral*>(vd.init.get())) {
                auto bytes = decodeStr(sl->value);
                for (uint32_t i = 0; i < s; ++i) {
                    int v = i < bytes.size() ? bytes[i] : 0;
                    current.body +=
                        "\tmov byte " + memOf(off + (int32_t)i) + ", " + std::to_string(v) + "\n";
                }
                return;
            }
        }
        if (auto l = layoutOf(vd.type.get())) {
            Reg dst = alloc();
            current.body += "\tlea " + R(dst) + ", " + memOf(off) + "\n";
            Reg src = lvalueAddr(*vd.init);
            emitMemCopy(dst, src, l->size);
            freeReg(dst);
            freeReg(src);
            return;
        }
        Reg r = emitRValue(*vd.init);
        emitStore(memOf(off), r, vd.type.get());
        freeReg(r);
    }
}

void CodeGenerator::visit(DeclStmt& node) {
    auto* vd = dynamic_cast<VarDecl*>(node.decl.get());
    if (vd) {
        emitLocalVar(*vd);
    }
}

inline bool isArithmetic(BinaryOp op) { return op >= BinaryOp::Add && op <= BinaryOp::Mod; }
inline bool isComparison(BinaryOp op) {
    return op >= BinaryOp::Less && op <= BinaryOp::GreaterEqual;
}
inline bool isLogical(BinaryOp op) { return op == BinaryOp::And || op == BinaryOp::Or; }
inline bool isAssign(BinaryOp op) { return op >= BinaryOp::Assign; }

void CodeGenerator::visit(BinaryExpr& node) {
    if (isAssign(node.op)) {
        if (auto l = layoutOf(node.left->resultType.get())) {
            Reg dst = lvalueAddr(*node.left);
            Reg src = lvalueAddr(*node.right);
            emitMemCopy(dst, src, l->size);
            freeReg(src);
            resultReg = dst;
            return;
        }
        LValue lv = emitLValue(*node.left);
        Reg r = emitRValue(*node.right);
        Type* lt = node.left->resultType.get();
        if (node.op == BinaryOp::Assign) {
            emitStore(lv.mem, r, lt);
        } else if (regClassOf(lt) == RegClass::Sse) {
            const char* m = node.op == BinaryOp::AddAssign     ? "addsd"
                            : node.op == BinaryOp::MinusAssign ? "subsd"
                            : node.op == BinaryOp::MulAssign   ? "mulsd"
                                                               : "divsd";
            Reg t = alloc(RegClass::Sse);
            current.body += "\tmovsd " + R(t) + ", " + lv.mem + "\n";
            current.body += "\t" + std::string(m) + " " + R(t) + ", " + R(r) + "\n";
            current.body += "\tmovsd " + lv.mem + ", " + R(t) + "\n";
            freeReg(t);
        } else {
            Reg t = alloc();
            emitLoad(t, lv.mem, lt);
            switch (node.op) {
            case BinaryOp::AddAssign:
                current.body += "\tadd " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::MinusAssign:
                current.body += "\tsub " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::MulAssign:
                current.body += "\timul " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitAndAssign:
                current.body += "\tand " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitOrAssign:
                current.body += "\tor " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitXorAssign:
                current.body += "\txor " + R(t) + ", " + R(r) + "\n";
                break;
            case BinaryOp::ShlAssign:
            case BinaryOp::ShrAssign:
                current.body += "\tmov rcx, " + R(r) + "\n";
                current.body += (node.op == BinaryOp::ShlAssign ? "\tsal "
                                 : isUnsigned(lt)               ? "\tshr "
                                                                : "\tsar ") +
                                R(t) + ", cl\n";
                break;
            case BinaryOp::DivAssign:
            case BinaryOp::ModAssign:
                current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
                emitRtCheck("jnz", "division by zero", node.offset);
                if (isUnsigned(lt))
                    current.body += "\tmov rax, " + R(t) + "\n\txor edx, edx\n\tdiv " + R(r) + "\n";
                else
                    current.body += "\tmov rax, " + R(t) + "\n\tcqo\n\tidiv " + R(r) + "\n";
                current.body += "\tmov " + R(t) + ", " +
                                (node.op == BinaryOp::DivAssign ? "rax" : "rdx") + "\n";
                break;
            default:
                break;
            }
            emitStore(lv.mem, t, lt);
            freeReg(t);
        }
        if (lv.ownsReg)
            freeReg(lv.reg);
        resultReg = r;
    } else if (isLogical(node.op)) {
        std::string endL = newLabel("logic");
        Reg res = emitRValue(*node.left);
        current.body += "\tcmp " + R(res) + ", 0\n";
        current.body += (node.op == BinaryOp::And ? "\tje " : "\tjne ") + endL + "\n";
        Reg r = emitRValue(*node.right);
        current.body += "\tmov " + R(res) + ", " + R(r) + "\n";
        freeReg(r);
        current.body += endL + ":\n";
        current.body += "\tcmp " + R(res) + ", 0\n\tsetne al\n\tmovzx " + R(res) + ", al\n";
        resultReg = res;
        return;
    } else {
        RegClass cls = regClassOf(node.left->resultType.get());
        Reg l = emitRValue(*node.left);
        Reg r = emitRValue(*node.right);

        if (isComparison(node.op)) {
            if (cls == RegClass::Sse) {
                // IEEE: NaN is unordered. ucomisd sets CF=ZF=PF=1 when unordered.
                // seta/setae need CF=0, so they are already false for NaN; emit
                // < / <= as swapped > / >=. ==/!= additionally test parity (PF).
                Reg res = alloc(RegClass::Int);
                switch (node.op) {
                case BinaryOp::Greater:
                    current.body += "\tucomisd " + R(l) + ", " + R(r) + "\n\tseta al\n";
                    break;
                case BinaryOp::GreaterEqual:
                    current.body += "\tucomisd " + R(l) + ", " + R(r) + "\n\tsetae al\n";
                    break;
                case BinaryOp::Less: // l < r  ==  r > l
                    current.body += "\tucomisd " + R(r) + ", " + R(l) + "\n\tseta al\n";
                    break;
                case BinaryOp::LessEqual: // l <= r ==  r >= l
                    current.body += "\tucomisd " + R(r) + ", " + R(l) + "\n\tsetae al\n";
                    break;
                case BinaryOp::Equal: // ordered AND equal
                    current.body += "\tucomisd " + R(l) + ", " + R(r) +
                                    "\n\tsete al\n\tsetnp cl\n\tand al, cl\n";
                    break;
                case BinaryOp::NotEqual: // unordered OR unequal
                    current.body += "\tucomisd " + R(l) + ", " + R(r) +
                                    "\n\tsetne al\n\tsetp cl\n\tor al, cl\n";
                    break;
                default:
                    break;
                }
                current.body += "\tmovzx " + R(res) + ", al\n";
                freeReg(l);
                freeReg(r);
                resultReg = res;
            } else {
                bool u = isUnsigned(node.left->resultType.get()) ||
                         isUnsigned(node.right->resultType.get());
                current.body += "\tcmp " + R(l) + ", " + R(r) + "\n";
                current.body += "\t" + std::string(u ? uintCC(node.op) : intCC(node.op)) + " al\n";
                current.body += "\tmovzx " + std::string(regName(l)) + ", al\n";
                freeReg(r);
                resultReg = l;
            }
            return;
        }
        if (cls == RegClass::Sse) {
            const char* m = node.op == BinaryOp::Add   ? "addsd"
                            : node.op == BinaryOp::Sub ? "subsd"
                            : node.op == BinaryOp::Mul ? "mulsd"
                                                       : "divsd";
            current.body += "\t" + std::string(m) + " " + R(l) + ", " + R(r) + "\n";
        } else {
            switch (node.op) {
            case BinaryOp::Add:
                if (auto pt = dynamic_cast<PointerType*>(node.left->resultType.get())) {
                    uint32_t esz = sizeOf(pt->base.get());
                    if (esz > 1)
                        current.body += "\timul " + R(r) + ", " + std::to_string(esz) + "\n";
                }
                current.body += "\tadd " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::Sub:
                if (auto pt = dynamic_cast<PointerType*>(node.left->resultType.get())) {
                    uint32_t esz = sizeOf(pt->base.get());
                    if (esz > 1)
                        current.body += "\timul " + R(r) + ", " + std::to_string(esz) + "\n";
                }
                current.body += "\tsub " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::Mul:
                current.body += "\timul " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitAnd:
                current.body += "\tand " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitOr:
                current.body += "\tor " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::BitXor:
                current.body += "\txor " + R(l) + ", " + R(r) + "\n";
                break;
            case BinaryOp::Shl:
            case BinaryOp::Shr:
                // cl, masked to 6 bits
                current.body += "\tmov rcx, " + R(r) + "\n";
                current.body += (node.op == BinaryOp::Shl                  ? "\tsal "
                                 : isUnsigned(node.left->resultType.get()) ? "\tshr "
                                                                           : "\tsar ") +
                                R(l) + ", cl\n";
                break;
            case BinaryOp::Div:
            case BinaryOp::Mod:
                current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
                emitRtCheck("jnz", "division by zero", node.offset);
                if (isUnsigned(node.left->resultType.get()))
                    current.body += "\tmov rax, " + R(l) + "\n\txor edx, edx\n\tdiv " + R(r) + "\n";
                else
                    current.body += "\tmov rax, " + R(l) + "\n\tcqo\n\tidiv " + R(r) + "\n";
                current.body +=
                    "\tmov " + R(l) + ", " + (node.op == BinaryOp::Div ? "rax" : "rdx") + "\n";
                break;
            default:
                break;
            }
        }
        freeReg(r);
        resultReg = l;
    }
}

void CodeGenerator::visit(UnaryExpr& node) {
    if (node.op == UnaryOp::Deref) {
        resultReg = loadLValue(emitLValue(node), node.resultType.get());
        return;
    }
    if (node.op == UnaryOp::AddressOf) {
        LValue lv = emitLValue(*node.child);
        if (lv.ownsReg) {
            resultReg = lv.reg;
        } else {
            Reg r = alloc();
            current.body += "\tlea " + R(r) + ", " + lv.mem + "\n";
            resultReg = r;
        }
        return;
    }
    if (node.op == UnaryOp::Neg) {
        Reg r = emitRValue(*node.child);
        if (regClassOf(node.resultType.get()) == RegClass::Sse) {
            Reg z = alloc(RegClass::Sse);
            current.body += "\txorpd " + R(z) + ", " + R(z) + "\n";
            current.body += "\tsubsd " + R(z) + ", " + R(r) + "\n"; // 0.0 - x
            freeReg(r);
            resultReg = z;
        } else {
            current.body += "\tneg " + R(r) + "\n";
            resultReg = r;
        }
        return;
    }
    if (node.op == UnaryOp::Not) {
        Reg r = emitRValue(*node.child);
        current.body += "\tcmp " + R(r) + ", 0\n\tsete al\n\tmovzx " + R(r) + ", al\n";
        resultReg = r;
        return;
    }
    if (node.op == UnaryOp::BitNot) {
        Reg r = emitRValue(*node.child);
        current.body += "\tnot " + R(r) + "\n";
        resultReg = r;
        return;
    }
    LValue lv = emitLValue(*node.child);
    Type* t = node.child->resultType.get();
    uint32_t step = 1;
    if (auto p = dynamic_cast<PointerType*>(t))
        step = sizeOf(p->base.get());
    bool inc = node.op == UnaryOp::PreInc || node.op == UnaryOp::PostInc;
    bool post = node.op == UnaryOp::PostInc || node.op == UnaryOp::PostDec;

    Reg val = alloc();
    emitLoad(val, lv.mem, t);
    Reg old = val;
    if (post) {
        old = alloc();
        current.body += "\tmov " + R(old) + ", " + R(val) + "\n";
    }
    current.body += (inc ? "\tadd " : "\tsub ") + R(val) + ", " + std::to_string(step) + "\n";
    emitStore(lv.mem, val, t);
    if (post)
        freeReg(val);
    if (lv.ownsReg)
        freeReg(lv.reg);
    resultReg = old;
}

void CodeGenerator::visit(CallExpr& node) {
    static const Reg argReg[6] = {Reg::rdi, Reg::rsi, Reg::rdx, Reg::rcx, Reg::r8, Reg::r9};
    auto* id = dynamic_cast<Identifier*>(node.func.get());
    bool direct = id && !findVar(id->name);
    std::size_t n = node.param.size();

    if (direct && id->name == "assert" && !funcs.count("assert")) {
        Reg r = emitRValue(*node.param[0]);
        current.body += "\ttest " + R(r) + ", " + R(r) + "\n";
        emitRtCheck("jnz", "assertion failed", node.offset);
        resultReg = r;
        return;
    }

    std::string callee = direct ? std::string(id->name) : "";
    bool isPanic = false;
    if (direct && !funcs.count(id->name)) {
        if (callee == "print")
            callee = "printf";
        else if (callee == "input")
            callee = "scanf";
        else if (callee == "panic") {
            callee = "puts";
            isPanic = true;
            usedExterns.insert("exit");
        }
        usedExterns.insert(callee);
    }

    std::vector<Reg> spill = current.inUse;
    for (Reg r : spill)
        current.body += "\tpush " + std::string(regName(r)) + "\n";
    std::vector<Reg> spillX = current.inUseXmm;
    for (Reg r : spillX)
        current.body += "\tsub rsp, 8\n\tmovsd [rsp], " + R(r) + "\n";
    current.pushDepth += spill.size() + spillX.size();

    std::size_t fpSlot = 0;
    if (!direct) {
        Reg r = emitRValue(*node.func);
        current.body += "\tsub rsp, 8\n\tmov [rsp], " + R(r) + "\n";
        ++current.pushDepth;
        fpSlot = 1;
        freeReg(r);
    }

    std::vector<RegClass> cls(n);
    for (std::size_t i = 0; i < n; ++i) {
        cls[i] = regClassOf(node.param[i]->resultType.get());
        Reg r = emitRValue(*node.param[i]);
        current.body += "\tsub rsp, 8\n";
        ++current.pushDepth;
        if (cls[i] == RegClass::Sse)
            current.body += "\tmovsd [rsp], " + R(r) + "\n";
        else
            current.body += "\tmov [rsp], " + R(r) + "\n";
        freeReg(r);
    }

    std::size_t gp = 0, sse = 0;
    std::vector<std::size_t> onStack;
    for (std::size_t i = 0; i < n; ++i) {
        std::string slot = "[rsp + " + std::to_string(8 * (n - 1 - i)) + "]";
        if (cls[i] == RegClass::Sse) {
            if (sse < 8)
                current.body += "\tmovsd xmm" + std::to_string(sse++) + ", " + slot + "\n";
            else
                onStack.push_back(i);
        } else {
            if (gp < 6)
                current.body += "\tmov " + std::string(regName(argReg[gp++])) + ", " + slot + "\n";
            else
                onStack.push_back(i);
        }
    }

    bool fill = ((current.pushDepth + onStack.size()) % 2) != 0;
    if (fill) {
        current.body += "\tsub rsp, 8\n";
        ++current.pushDepth;
    }

    std::size_t sh = fill ? 1 : 0;
    for (std::size_t k = onStack.size(); k-- > 0;) {
        std::size_t i = onStack[k];
        current.body += "\tpush qword [rsp + " + std::to_string(8 * (n - 1 - i + sh)) + "]\n";
        ++sh;
        ++current.pushDepth;
    }

    current.body += "\tmov eax, " + std::to_string(sse) + "\n"; // varargs: al = sse args
    if (direct) {
        current.body += "\tcall " + callee + "\n";
        if (isPanic)
            current.body += "\tmov edi, 1\n\tcall exit\n"; // never returns
    } else {
        current.body += "\tmov r10, [rsp + " + std::to_string(8 * (n + sh)) + "]\n";
        current.body += "\tcall r10\n";
    }
    current.hasCall = true;

    std::size_t cleanup = n + fpSlot + onStack.size() + (fill ? 1 : 0);
    if (cleanup)
        current.body += "\tadd rsp, " + std::to_string(8 * cleanup) + "\n";
    current.pushDepth -= cleanup;

    for (std::size_t i = spillX.size(); i-- > 0;)
        current.body += "\tmovsd " + R(spillX[i]) + ", [rsp]\n\tadd rsp, 8\n";
    for (std::size_t i = spill.size(); i-- > 0;)
        current.body += "\tpop " + std::string(regName(spill[i])) + "\n";
    current.pushDepth -= spill.size() + spillX.size();

    Reg res = alloc(regClassOf(node.resultType.get()));
    if (isXmm(res))
        current.body += "\tmovsd " + R(res) + ", xmm0\n";
    else
        current.body += "\tmov " + R(res) + ", rax\n";
    resultReg = res;
}

void CodeGenerator::visit(CastExpr& node) {
    Reg src = emitRValue(*node.expr);
    Type* to = node.target.get();
    RegClass fromCls = regClassOf(node.expr->resultType.get());
    RegClass toCls = regClassOf(to);

    if (fromCls == toCls) {
        if (toCls == RegClass::Int) {
            uint32_t s = sizeOf(to);
            if (auto b = dynamic_cast<BuiltinType*>(to); b && b->type == BuiltinTypes::Bool)
                current.body += "\tcmp " + R(src) + ", 0\n\tsetne al\n\tmovzx " + R(src) + ", al\n";
            else if (s < 8)
                current.body += "\tmovsx " + R(src) + ", " + R(src, s) + "\n";
        }
        resultReg = src;
        return;
    }
    Reg dst = alloc(toCls);
    if (toCls == RegClass::Sse)
        current.body += "\tcvtsi2sd " + R(dst) + ", " + R(src) + "\n";
    else
        current.body += "\tcvttsd2si " + R(dst) + ", " + R(src) + "\n";
    freeReg(src);
    resultReg = dst;
}

Reg CodeGenerator::emitBaseAddr(Expr& arr) {
    if (dynamic_cast<ArrayType*>(arr.resultType.get())) {
        LValue lv = emitLValue(arr);
        Reg r = alloc();
        current.body += "\tlea " + R(r) + ", " + lv.mem + "\n";
        if (lv.ownsReg)
            freeReg(lv.reg);
        return r;
    }
    return emitRValue(arr);
}

void CodeGenerator::visit(IndexExpr& node) {
    resultReg = loadLValue(emitLValue(node), node.resultType.get());
}

void CodeGenerator::visit(AccessExpr& node) {
    resultReg = loadLValue(emitLValue(node), node.resultType.get());
}

static std::string typeStr(Type* t) {
    if (auto b = dynamic_cast<BuiltinType*>(t)) {
        switch (b->type) {
        case BuiltinTypes::Int:
            return "int";
        case BuiltinTypes::UInt:
            return "uint";
        case BuiltinTypes::Short:
            return "short";
        case BuiltinTypes::Long:
            return "long";
        case BuiltinTypes::Float:
            return "float";
        case BuiltinTypes::Char:
            return "char";
        case BuiltinTypes::Bool:
            return "bool";
        case BuiltinTypes::Void:
            return "void";
        case BuiltinTypes::Byte:
            return "byte";
        case BuiltinTypes::Custom:
            return std::string(b->name);
        }
    }
    if (auto p = dynamic_cast<PointerType*>(t))
        return typeStr(p->base.get()) + "*";
    if (auto a = dynamic_cast<ArrayType*>(t))
        return typeStr(a->elemType.get()) + "[" + std::to_string(a->size) + "]";
    if (auto f = dynamic_cast<FuncType*>(t)) {
        std::string s = typeStr(f->returnType.get()) + "(";
        for (std::size_t i = 0; i < f->params.size(); ++i)
            s += (i ? ", " : "") + typeStr(f->params[i].get());
        return s + ")";
    }
    return "?";
}

void CodeGenerator::visit(SizeofExpr& node) {
    Type* t = node.target ? node.target.get() : node.expr->resultType.get();
    Reg r = alloc();
    current.body += "\tmov " + R(r) + ", " + std::to_string(sizeOf(t)) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(TypeidExpr& node) {
    std::string lbl = "Lstr" + std::to_string(labelId++);
    rodataBuf += lbl + ": db \"" + typeStr(node.expr->resultType.get()) + "\", 0\n";
    Reg r = alloc();
    current.body += "\tlea " + R(r) + ", [rel " + lbl + "]\n";
    resultReg = r;
}

void CodeGenerator::visit(IntLiteral& node) {
    Reg r = alloc(regClassOf(node.resultType.get()));
    current.body += "\tmov " + std::string(regName(r)) + ", " + std::to_string(node.value) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(FloatLiteral& node) {
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

void CodeGenerator::visit(CharLiteral& node) {
    Reg r = alloc(RegClass::Int);
    current.body += "\tmov " + R(r) + ", " + std::to_string((int)node.value) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(BoolLiteral& node) {
    Reg r = alloc(regClassOf(node.resultType.get()));
    current.body +=
        "\tmov " + std::string(regName(r)) + ", " + std::to_string(node.value ? 1 : 0) + "\n";
    resultReg = r;
}

void CodeGenerator::visit(NullLiteral&) {
    Reg r = alloc();
    current.body += "\tmov " + R(r) + ", 0\n";
    resultReg = r;
}

void CodeGenerator::visit(StringLiteral& node) {
    std::string lbl = "Lstr" + std::to_string(labelId++);
    rodataBuf += lbl + ": db ";
    bool first = true;
    for (uint8_t b : decodeStr(node.value)) {
        rodataBuf += (first ? "" : ", ") + std::to_string((int)b);
        first = false;
    }
    rodataBuf += "\n";
    Reg r = alloc();
    current.body += "\tlea " + std::string(regName(r)) + ", [rel " + lbl + "]\n";
    resultReg = r;
}

void CodeGenerator::visit(Identifier& node) {
    if (!findVar(node.name) && funcs.count(node.name)) {
        Reg r = alloc();
        current.body += "\tlea " + R(r) + ", [rel " + std::string(node.name) + "]\n";
        resultReg = r;
        return;
    }
    resultReg = loadLValue(emitLValue(node), node.resultType.get());
}

void CodeGenerator::visit(BuiltinType&) {}

void CodeGenerator::visit(PointerType&) {}

void CodeGenerator::visit(FuncType&) {}

void CodeGenerator::visit(ArrayType&) {}
