#include "SemanticAnalyzer.h"

SemanticAnalyzer::SemanticAnalyzer() { enterScope(); }
SemanticAnalyzer::~SemanticAnalyzer() {}

void SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<ASTNode>>& ast)
{
    for (const auto& node : ast) node->accept(*this);
}


void SemanticAnalyzer::visit(const NumberNode&) { m_currentType = VarType::INT;    }
void SemanticAnalyzer::visit(const FloatNode&)  { m_currentType = VarType::FLOAT;  }
void SemanticAnalyzer::visit(const BoolNode&)   { m_currentType = VarType::BOOL;   }
void SemanticAnalyzer::visit(const StringNode&) { m_currentType = VarType::STRING; }

std::string SemanticAnalyzer::varTypeToString(VarType vt)
{   
    switch (vt) {
        case VarType::INT:    return "int";
        case VarType::FLOAT:  return "float";
        case VarType::STRING: return "string";
        case VarType::BOOL:   return "bool";
        case VarType::VOID:   return "void";
        case VarType::ARRAY:  return "array";
        default:              return "unknown";
    }
}

void SemanticAnalyzer::visit(const CastNode& node)
{
    node.expression->accept(*this);
    VarType targetType = stringToVarType(node.targetType);
    if (targetType == m_currentType)
        m_currentType = targetType;
    else if (targetType == VarType::FLOAT && m_currentType == VarType::INT)
        m_currentType = VarType::FLOAT;
    else if (targetType == VarType::INT && m_currentType == VarType::FLOAT)
        m_currentType = VarType::INT;
    else if (targetType == VarType::INT && m_currentType == VarType::STRING)
        m_currentType = VarType::INT;
    else if (targetType == VarType::FLOAT && m_currentType == VarType::STRING)
        m_currentType = VarType::FLOAT;
    else
        throw std::runtime_error("Invalid cast from " + varTypeToString(m_currentType) + " to " + node.targetType + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
}

void SemanticAnalyzer::visit(const LetNode& node)
{
    if (node.hasTypeAnnotation && node.isArray) {
        VarType elemType = stringToVarType(node.annotatedType);
        int     size     = node.arraySize;

        if (node.value) {
            auto* lit  = dynamic_cast<const ArrayLiteralNode*>(node.value.get());
            auto* copy = dynamic_cast<const CopyNode*>(node.value.get());

            if (copy) {
                copy->accept(*this);
                if (m_currentType != VarType::ARRAY)
                    throw std::runtime_error("copy() source is not an array at line " + std::to_string(node.line));
                if (m_currentElementType != elemType)
                    throw std::runtime_error("Array element type mismatch in copy() at line " + std::to_string(node.line));
            } else if (lit) {
                if (size == -1)
                    size = static_cast<int>(lit->elements.size());
                else if (static_cast<int>(lit->elements.size()) != size)
                    throw std::runtime_error(
                        "Array '" + node.variableName + "' size mismatch at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

                for (const auto& elem : lit->elements) {
                    elem->accept(*this);
                    if (m_currentType == elemType)
                        continue;
                    if (elemType == VarType::FLOAT && m_currentType == VarType::INT)
                        continue;
                    throw std::runtime_error(
                        "Type mismatch in array literal for '" + node.variableName + "' at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
                }
            } else {
                throw std::runtime_error(
                    "Array '" + node.variableName + "' must be initialised with an array literal or copy() at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
            }
        } else if (size <= 0) {
            throw std::runtime_error(
                "Array '" + node.variableName + "' without initialiser needs explicit size at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
        }

        if (!declareVariable(node.variableName, VarType::ARRAY, node.isMutable, elemType, size))
            throw std::runtime_error("Variable already declared: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

        m_currentType = VarType::ARRAY;
        return;
    }

    // Scalar
    if (!node.value)
        throw std::runtime_error("Variable '" + node.variableName + "' must have an initialiser at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    node.value->accept(*this);
    VarType varType = m_currentType;

    if (node.hasTypeAnnotation) {
        VarType annotated = stringToVarType(node.annotatedType);
        if (varType == annotated) {
            varType = annotated;
        } else if (annotated == VarType::FLOAT && varType == VarType::INT) {
            varType = VarType::FLOAT;
        } else {
            throw std::runtime_error("Type mismatch in declaration of '" + node.variableName + "' at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
        }
    }

    // If RHS is a variable of heap type, that's a move — mark source as moved
    if (isHeapType(varType)) {
        if (auto* varNode = dynamic_cast<const VariableNode*>(node.value.get())) {
            markAsMoved(varNode->name);
        }
    }

    if (varType == VarType::ARRAY) {
        if (!declareVariable(node.variableName, varType, node.isMutable, m_currentElementType, -1))
            throw std::runtime_error("Variable already declared: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    } else {
        if (!declareVariable(node.variableName, varType, node.isMutable))
            throw std::runtime_error("Variable already declared: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    }
}


void SemanticAnalyzer::visit(const ArrayLiteralNode& node)
{
    if (node.elements.empty())
        throw std::runtime_error("Array literal must not be empty at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    node.elements[0]->accept(*this);
    VarType firstType = m_currentType;

    for (size_t i = 1; i < node.elements.size(); i++) {
        node.elements[i]->accept(*this);
        if (m_currentType != firstType)
            throw std::runtime_error("All elements of an array literal must have the same type at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    }

    m_currentElementType = firstType;
    m_currentType        = VarType::ARRAY;
}

void SemanticAnalyzer::visit(const ArrayAccessNode& node)
{
    auto var = lookupVariable(node.name);
    if (!var) throw std::runtime_error("Undefined variable: " + node.name + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    validateNotMoved(node.name, node.line, node.column);
    if (var->get().type != VarType::ARRAY)
        throw std::runtime_error("'" + node.name + "' is not an array at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    node.index->accept(*this);
    if (m_currentType != VarType::INT)
        throw std::runtime_error("Array index must be int at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    m_currentElementType = var->get().elementType;
    m_currentType        = var->get().elementType;
}

void SemanticAnalyzer::visit(const ArrayAssignNode& node)
{
    auto var = lookupVariable(node.name);
    if (!var) throw std::runtime_error("Undefined variable: " + node.name + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    validateNotMoved(node.name, node.line, node.column);
    if (var->get().type != VarType::ARRAY)
        throw std::runtime_error("'" + node.name + "' is not an array at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    if (!var->get().mutableFlag)
        throw std::runtime_error("Cannot assign to immutable array: " + node.name + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    node.index->accept(*this);
    if (m_currentType != VarType::INT)
        throw std::runtime_error("Array index must be int at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));

    node.value->accept(*this);
    if (m_currentType != var->get().elementType)
        throw std::runtime_error("Type mismatch in array assignment to '" + node.name + "' at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
}


void SemanticAnalyzer::visit(const AssignmentNode& node)
{
    auto var = lookupVariable(node.variableName);
    if (!var) throw std::runtime_error("Undefined variable: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    if (!var->get().mutableFlag)
        throw std::runtime_error("Cannot assign to immutable variable: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
    
    // Validate ownership: can't assign to moved or borrowed values
    validateCanMutate(node.variableName, node.line, node.column);
    
    node.expression->accept(*this);
    
    // For heap types, this assignment moves the value and reinitializes the target if it was moved.
    if (var->get().isHeapType()) {
        if (var->get().ownershipState == OwnershipState::MOVED)
            var->get().ownershipState = OwnershipState::OWNED;
        clearBorrows(node.variableName);
    }
    
    if (var->get().type == m_currentType)
        return;
    if (var->get().type == VarType::FLOAT && m_currentType == VarType::INT)
        return;
    throw std::runtime_error("Type mismatch in assignment to: " + node.variableName + " at line " + std::to_string(node.line) + ", column " + std::to_string(node.column));
}

void SemanticAnalyzer::visit(const BinaryOpNode& node)
{
    node.left->accept(*this);  VarType leftType  = m_currentType;
    node.right->accept(*this); VarType rightType = m_currentType;

    auto isNumeric = [&](VarType t) {
        return t == VarType::INT || t == VarType::FLOAT;
    };

    switch (node.op) {
        case TokenType::PLUS: case TokenType::MINUS:
        case TokenType::STAR: case TokenType::SLASH:
            if (!isNumeric(leftType) || !isNumeric(rightType))
                throw std::runtime_error("Arithmetic operations require numeric types");
            if (node.op == TokenType::SLASH) {
                if (auto* lit = dynamic_cast<const NumberNode*>(node.right.get())) {
                    if (lit->value == 0 && leftType == VarType::INT && rightType == VarType::INT)
                        throw std::runtime_error("Division by zero");
                }
            }
            m_currentType = (leftType == VarType::FLOAT || rightType == VarType::FLOAT)
                                ? VarType::FLOAT
                                : VarType::INT;
            break;

        case TokenType::GREATER: case TokenType::LESS:
        case TokenType::GREATER_EQUAL: case TokenType::LESS_EQUAL:
            if (!isNumeric(leftType) || !isNumeric(rightType))
                throw std::runtime_error("Comparison operators require numeric types");
            m_currentType = VarType::BOOL; break;

        case TokenType::DOUBLE_EQUAL: case TokenType::NOT_EQUAL:
            if (leftType == rightType) {
                m_currentType = VarType::BOOL;
                break;
            }
            if ((leftType == VarType::INT && rightType == VarType::FLOAT) ||
                (leftType == VarType::FLOAT && rightType == VarType::INT)) {
                m_currentType = VarType::BOOL;
                break;
            }
            throw std::runtime_error("Equality comparison requires compatible types");

        case TokenType::LOGICAL_AND: case TokenType::LOGICAL_OR:
            if ((leftType  != VarType::BOOL && leftType  != VarType::INT) ||
                (rightType != VarType::BOOL && rightType != VarType::INT))
                throw std::runtime_error("Logical operations require bool or int");
            m_currentType = VarType::BOOL; break;

        default: throw std::runtime_error("Unsupported binary operator");
    }
}

void SemanticAnalyzer::visit(const VariableNode& node)
{
    auto var = lookupVariable(node.name);
    if (!var) throw std::runtime_error("Undefined variable: " + node.name);
    
    // Check ownership: can't use moved variables
    validateNotMoved(node.name, node.line, node.column);
    
    m_currentType = var->get().type;
    if (m_currentType == VarType::ARRAY)
        m_currentElementType = var->get().elementType;
}

void SemanticAnalyzer::visit(const BlockNode& node)
{
    enterScope();
    for (const auto& stmt : node.statements) stmt->accept(*this);
    exitScope();
}

void SemanticAnalyzer::visit(const IfNode& node)
{
    node.condition->accept(*this);
    if (m_currentType != VarType::BOOL && m_currentType != VarType::INT)
        throw std::runtime_error("If condition must be bool");
    node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(const WhileNode& node)
{
    node.condition->accept(*this);
    if (m_currentType != VarType::BOOL)
        throw std::runtime_error("While condition must be bool");
    node.body->accept(*this);
}

void SemanticAnalyzer::visit(const ForNode& node)
{
    if (node.isRange()) {
        node.iterable->accept(*this);
        if (m_currentType != VarType::INT)
            throw std::runtime_error("Range start expression in for loop must be int");

        node.endExpr->accept(*this);
        if (m_currentType != VarType::INT)
            throw std::runtime_error("Range end expression in for loop must be int");

        enterScope();
        if (!declareVariable(node.iteratorName, VarType::INT, true))
            throw std::runtime_error("Variable already declared in for loop: " + node.iteratorName);
        node.body->accept(*this);
        exitScope();
        return;
    }

    node.iterable->accept(*this);
    if (m_currentType != VarType::ARRAY)
        throw std::runtime_error("For loop iterable must be an array");

    VarType elementType = m_currentElementType;
    enterScope();
    if (!declareVariable(node.iteratorName, elementType, true))
        throw std::runtime_error("Variable already declared in for loop: " + node.iteratorName);
    node.body->accept(*this);
    exitScope();
}

void SemanticAnalyzer::visit(const UnaryOpNode& node)
{
    node.operand->accept(*this);
    switch (node.op) {
        case TokenType::NOT:
            if (m_currentType != VarType::BOOL && m_currentType != VarType::INT)
                throw std::runtime_error("NOT requires bool or int");
            m_currentType = VarType::BOOL; break;
        case TokenType::MINUS:
            if (m_currentType != VarType::INT && m_currentType != VarType::FLOAT)
                throw std::runtime_error("Unary minus requires int or float");
            break;
        default: throw std::runtime_error("Unsupported unary operator");
    }
}

void SemanticAnalyzer::visit(const FunctionNode& node)
{
    if (m_functionsTable.count(node.name))
        throw std::runtime_error("Function already declared: " + node.name);

    FunctionInfo info;
    for (auto& p : node.parameters) {
        ParamInfo pi;
        pi.isArray    = p.isArray;
        pi.borrowType = p.borrowType;
        if (p.isArray) {
            pi.varType  = VarType::ARRAY;
            pi.elemType = stringToVarType(p.type);
        } else {
            pi.varType  = stringToVarType(p.type);
            pi.elemType = VarType::INT;
        }
        info.params.push_back(pi);
    }
    info.returnType = stringToVarType(node.returnType);
    m_functionsTable[node.name] = info;

    m_currentFunctionReturnType = info.returnType;
    m_foundReturn               = false;
    m_insideFunction            = true;

    enterScope();
    for (size_t i = 0; i < node.parameters.size(); i++) {
        const auto& p  = node.parameters[i];
        const auto& pi = info.params[i];
        if (pi.isArray) {
            // Declare as ARRAY; size -1 (pointer — no compile-time size)
            declareVariable(p.name, VarType::ARRAY,
                            /*mutable=*/true, pi.elemType, /*size=*/-1);
        } else {
            declareVariable(p.name, pi.varType, /*mutable=*/false);
        }
    }

    node.body->accept(*this);
    exitScope();
    m_insideFunction = false;

    if (info.returnType != VarType::VOID && !m_foundReturn)
        throw std::runtime_error("Function '" + node.name + "' must return a value");
}

void SemanticAnalyzer::visit(const ReturnNode& node)
{
    if (!m_insideFunction)
        throw std::runtime_error("Return statement outside function");
    node.expression->accept(*this);
    if (m_currentType == m_currentFunctionReturnType) {
        m_foundReturn = true;
        return;
    }
    if (m_currentFunctionReturnType == VarType::FLOAT && m_currentType == VarType::INT) {
        m_foundReturn = true;
        return;
    }
    throw std::runtime_error("Return type mismatch");
}


void SemanticAnalyzer::visit(const CallNode& node)
{
    auto it = m_functionsTable.find(node.functionName);
    if (it == m_functionsTable.end())
        throw std::runtime_error("Call to undefined function: " + node.functionName);

    const FunctionInfo& info = it->second;
    if (node.arguments.size() != info.params.size())
        throw std::runtime_error(
            "Function '" + node.functionName + "' expects " +
            std::to_string(info.params.size()) + " arguments, got " +
            std::to_string(node.arguments.size()));

    for (size_t i = 0; i < node.arguments.size(); i++) {
        node.arguments[i]->accept(*this);
        const ParamInfo& pi = info.params[i];

        if (pi.isArray) {
            if (m_currentType != VarType::ARRAY)
                throw std::runtime_error(
                    "Argument " + std::to_string(i + 1) +
                    " to '" + node.functionName + "' must be an array");
            if (m_currentElementType != pi.elemType)
                throw std::runtime_error(
                    "Array element type mismatch for argument " +
                    std::to_string(i + 1) + " in call to '" + node.functionName + "'");
        } else {
            if (m_currentType != pi.varType) {
                if (pi.varType == VarType::FLOAT && m_currentType == VarType::INT) {
                    // ok: implicit int->float
                } else {
                    throw std::runtime_error(
                        "Type mismatch for argument " + std::to_string(i + 1) +
                        " in call to '" + node.functionName + "'");
                }
            }
        }

        // Ownership: if heap type arg passed by value (no borrow) → move
        if (isHeapType(m_currentType) && pi.borrowType.empty()) {
            if (auto* varNode = dynamic_cast<const VariableNode*>(node.arguments[i].get())) {
                validateNotMoved(varNode->name, node.line, node.column);
                markAsMoved(varNode->name);
            }
        }
        // Borrow at call site
        if (!pi.borrowType.empty()) {
            if (auto* varNode = dynamic_cast<const VariableNode*>(node.arguments[i].get())) {
                OwnershipState bs = (pi.borrowType == "mut") ? OwnershipState::BORROWED_MUT : OwnershipState::BORROWED_IMM;
                validateCanBorrow(varNode->name, bs, node.line, node.column);
                markAsBorrowed(varNode->name, bs);
            }
        }
    }

    m_currentType = info.returnType;
}

void SemanticAnalyzer::visit(const PrintNode& node)
{
    for (auto& arg : node.args) {
        if (!arg) throw std::runtime_error("Print argument is null");
        arg->accept(*this);
        if (m_currentType != VarType::INT    && m_currentType != VarType::BOOL &&
            m_currentType != VarType::STRING && m_currentType != VarType::FLOAT)
            throw std::runtime_error("Print only supports int, bool, float, or string");
    }
    m_currentType = VarType::VOID;
}

void SemanticAnalyzer::visit(const InputNode& node)
{
    m_currentType = VarType::STRING; // input is string by default, can be cast to other types by user
}


void SemanticAnalyzer::enterScope() { m_symbolTable.emplace_back(); }
void SemanticAnalyzer::exitScope()  { if (!m_symbolTable.empty()) m_symbolTable.pop_back(); }

bool SemanticAnalyzer::declareVariable(const std::string& name, VarType type,
                                        bool mutableFlag, VarType elementType, int arraySize)
{
    auto& scope = m_symbolTable.back();
    if (scope.count(name)) return false;
    scope[name] = VariableInfo{name, type, mutableFlag, elementType, arraySize};
    return true;
}

VarType SemanticAnalyzer::stringToVarType(const std::string& s)
{
    if (s == "int")    return VarType::INT;
    if (s == "float")  return VarType::FLOAT;
    if (s == "string") return VarType::STRING;
    if (s == "bool")   return VarType::BOOL;
    if (s == "void")   return VarType::VOID;
    throw std::runtime_error("Unknown type: " + s);
}



std::optional<std::reference_wrapper<VariableInfo>>
SemanticAnalyzer::lookupVariable(const std::string& name)
{
    for (auto it = m_symbolTable.rbegin(); it != m_symbolTable.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return std::ref(found->second);
    }
    return std::nullopt;
}
// ===== OWNERSHIP SYSTEM HELPERS =====

bool SemanticAnalyzer::isHeapType(VarType type) const
{
    return type == VarType::STRING || type == VarType::ARRAY;
}

void SemanticAnalyzer::validateNotMoved(const std::string& varName, int line, int col)
{
    auto var = lookupVariable(varName);
    if (!var) return;  // Already checked by lookupVariable elsewhere
    
    if (var->get().ownershipState == OwnershipState::MOVED) {
        throw std::runtime_error("Variable '" + varName + "' has been moved and cannot be used at line " + 
                                 std::to_string(line) + ", column " + std::to_string(col));
    }
}

void SemanticAnalyzer::validateCanBorrow(const std::string& varName, OwnershipState requestedBorrow, int line, int col)
{
    auto var = lookupVariable(varName);
    if (!var) return;
    
    VariableInfo& info = var->get();
    
    // Can't borrow moved values
    if (info.ownershipState == OwnershipState::MOVED) {
        throw std::runtime_error("Cannot borrow moved variable '" + varName + "' at line " + 
                                 std::to_string(line) + ", column " + std::to_string(col));
    }
    
    // Can't take mutable borrow while other borrows exist
    if (requestedBorrow == OwnershipState::BORROWED_MUT && !info.activeBorrows.empty()) {
        throw std::runtime_error("Cannot mutably borrow '" + varName + "' while it is already borrowed at line " + 
                                 std::to_string(line) + ", column " + std::to_string(col));
    }
    
    // Can't take immutable borrow while mutably borrowed
    if (requestedBorrow == OwnershipState::BORROWED_IMM) {
        for (const auto& borrow : info.activeBorrows) {
            if (borrow.state == OwnershipState::BORROWED_MUT) {
                throw std::runtime_error("Cannot immutably borrow '" + varName + "' while it is mutably borrowed at line " + 
                                         std::to_string(line) + ", column " + std::to_string(col));
            }
        }
    }
}

void SemanticAnalyzer::validateCanMutate(const std::string& varName, int line, int col)
{
    auto var = lookupVariable(varName);
    if (!var) return;
    
    VariableInfo& info = var->get();
    
    // Can't mutate immutable variables
    if (!info.mutableFlag) {
        throw std::runtime_error("Cannot mutate immutable variable '" + varName + "' at line " + 
                                 std::to_string(line) + ", column " + std::to_string(col));
    }
    
    // Can't mutate while borrowed
    if (!info.activeBorrows.empty()) {
        throw std::runtime_error("Cannot mutate '" + varName + "' while it is borrowed at line " + 
                                 std::to_string(line) + ", column " + std::to_string(col));
    }
}

void SemanticAnalyzer::markAsMoved(const std::string& varName)
{
    auto var = lookupVariable(varName);
    if (var && var->get().isHeapType()) {
        var->get().ownershipState = OwnershipState::MOVED;
        var->get().activeBorrows.clear();
    }
}

void SemanticAnalyzer::markAsBorrowed(const std::string& varName, OwnershipState borrowType)
{
    auto var = lookupVariable(varName);
    if (var && var->get().isHeapType()) {
        BorrowInfo borrow{borrowType, ""};
        var->get().activeBorrows.push_back(borrow);
    }
}

void SemanticAnalyzer::clearBorrows(const std::string& varName)
{
    auto var = lookupVariable(varName);
    if (var) {
        var->get().activeBorrows.clear();
        if (var->get().ownershipState == OwnershipState::BORROWED_IMM || 
            var->get().ownershipState == OwnershipState::BORROWED_MUT) {
            var->get().ownershipState = OwnershipState::OWNED;
        }
    }
}

void SemanticAnalyzer::visit(const BorrowNode& node)
{
    auto var = lookupVariable(node.name);
    if (!var)
        throw std::runtime_error("Undefined variable: '" + node.name + "' at line " +
                                 std::to_string(node.line) + ", column " + std::to_string(node.column));
    validateNotMoved(node.name, node.line, node.column);

    OwnershipState bs = node.isMutable ? OwnershipState::BORROWED_MUT : OwnershipState::BORROWED_IMM;
    validateCanBorrow(node.name, bs, node.line, node.column);
    markAsBorrowed(node.name, bs);

    // The type of &T / &mut T is still T from our type system perspective
    m_currentType = var->get().type;
    if (m_currentType == VarType::ARRAY)
        m_currentElementType = var->get().elementType;
}

void SemanticAnalyzer::visit(const CopyNode& node)
{
    auto var = lookupVariable(node.name);
    if (!var)
        throw std::runtime_error("Undefined variable: '" + node.name + "' at line " +
                                 std::to_string(node.line) + ", column " + std::to_string(node.column));
    validateNotMoved(node.name, node.line, node.column);

    if (!var->get().isHeapType())
        throw std::runtime_error("copy() is for heap types (string/array). Use direct assignment for stack types at line " +
                                 std::to_string(node.line));

    m_currentType = var->get().type;
    if (m_currentType == VarType::ARRAY)
        m_currentElementType = var->get().elementType;
}