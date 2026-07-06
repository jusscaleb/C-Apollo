import React, { useState } from 'react';
import { 
  BookOpen, 
  Cpu, 
  Layers, 
  Terminal, 
  ChevronRight, 
  Play, 
  AlertCircle, 
  FileText, 
  Settings, 
  Code,
  RefreshCw,
  GitBranch,
  HelpCircle
} from 'lucide-react';
import './App.css';

// ==========================================
// 1. APOLLO INTERACTIVE SIMULATOR (JS-BASED)
// ==========================================

// Tokenize Apollo source code in JS
function jsLex(source) {
  const tokens = [];
  let i = 0;
  let line = 1;

  const keywords = {
    'fxn': 'TOKEN_FXN',
    'run': 'TOKEN_RUN',
    'void': 'TOKEN_VOID',
    'println': 'TOKEN_PRINTLN',
    'var': 'TOKEN_VAR',
    'true': 'TOKEN_BOOL',
    'false': 'TOKEN_BOOL',
    'null': 'TOKEN_NULL',
    'and': 'TOKEN_AND',
    'or': 'TOKEN_OR',
    'if': 'TOKEN_IF',
    'else': 'TOKEN_ELSE',
    'elif': 'TOKEN_ELIF',
    'while': 'TOKEN_WHILE',
    'for': 'TOKEN_FOR'
  };

  while (i < source.length) {
    let char = source[i];

    // Newlines
    if (char === '\n') {
      line++;
      i++;
      continue;
    }

    // Whitespace
    if (/\s/.test(char)) {
      i++;
      continue;
    }

    // Comments
    if (char === '/' && source[i+1] === '/') {
      while (i < source.length && source[i] !== '\n') {
        i++;
      }
      continue;
    }

    // Multi-char operators/arrows
    if (source.slice(i, i + 2) === '->') {
      tokens.push({ type: 'TOKEN_ARROW', value: '->', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '==') {
      tokens.push({ type: 'TOKEN_EQT', value: '==', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '!=') {
      tokens.push({ type: 'TOKEN_NEQ', value: '!=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '+=') {
      tokens.push({ type: 'TOKEN_AEQ', value: '+=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '-=') {
      tokens.push({ type: 'TOKEN_SEQ', value: '-=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '*=') {
      tokens.push({ type: 'TOKEN_MEQ', value: '*=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '/=') {
      tokens.push({ type: 'TOKEN_DEQ', value: '/=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '++') {
      tokens.push({ type: 'TOKEN_INC', value: '++', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '--') {
      tokens.push({ type: 'TOKEN_DEC', value: '--', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '<=') {
      tokens.push({ type: 'TOKEN_SE', value: '<=', line });
      i += 2;
      continue;
    }
    if (source.slice(i, i + 2) === '>=') {
      tokens.push({ type: 'TOKEN_GE', value: '>=', line });
      i += 2;
      continue;
    }

    // String literals
    if (char === '"') {
      let start = i;
      i++; // consume opening quote
      let val = "";
      while (i < source.length && source[i] !== '"') {
        val += source[i];
        i++;
      }
      if (i >= source.length) {
        throw new Error(`Lexical Error: Unterminated string on line ${line}`);
      }
      i++; // consume closing quote
      tokens.push({ type: 'TOKEN_STRING', value: val, line });
      continue;
    }

    // Numbers
    if (/[0-9]/.test(char)) {
      let val = "";
      let isFloat = false;
      while (i < source.length && /[0-9\.]/.test(source[i])) {
        if (source[i] === '.') {
          if (isFloat) break; // Invalid format
          isFloat = true;
        }
        val += source[i];
        i++;
      }
      tokens.push({ 
        type: isFloat ? 'TOKEN_FLOAT' : 'TOKEN_INT', 
        value: val, 
        line 
      });
      continue;
    }

    // Identifiers & Keywords
    if (/[a-zA-Z_]/.test(char)) {
      let val = "";
      while (i < source.length && /[a-zA-Z0-9_]/.test(source[i])) {
        val += source[i];
        i++;
      }
      if (keywords[val]) {
        tokens.push({ type: keywords[val], value: val, line });
      } else {
        tokens.push({ type: 'TOKEN_IDENTIFIER', value: val, line });
      }
      continue;
    }

    // Single-char symbols
    const singleSymbols = {
      ';': 'TOKEN_SEMICOLON',
      '(': 'TOKEN_LPARETH',
      ')': 'TOKEN_RPARETH',
      '{': 'TOKEN_LBRACE',
      '}': 'TOKEN_RBRACE',
      '=': 'TOKEN_ASSIGN',
      '+': 'TOKEN_ADD',
      '-': 'TOKEN_SUB',
      '*': 'TOKEN_MUL',
      '/': 'TOKEN_DIV',
      '%': 'TOKEN_MOD',
      '<': 'TOKEN_ST',
      '>': 'TOKEN_GT'
    };

    if (singleSymbols[char]) {
      tokens.push({ type: singleSymbols[char], value: char, line });
      i++;
      continue;
    }

    throw new Error(`Lexical Error: Unexpected character '${char}' on line ${line}`);
  }

  tokens.push({ type: 'TOKEN_EOF', value: '\\0', line });
  return tokens;
}

// Build basic AST in JS
function jsParse(tokens) {
  let index = 0;
  
  function peek() {
    return tokens[index];
  }
  
  function consume(expectedType, errMsg) {
    const t = peek();
    if (t && t.type === expectedType) {
      index++;
      return t;
    }
    throw new Error(`Syntax Error: ${errMsg || `Expected ${expectedType}`} on line ${t ? t.line : 'EOF'} (Found: ${t ? t.value : 'EOF'})`);
  }

  function parseExpression() {
    // Basic math expression parser
    let left = parsePrimary();
    
    while (peek() && ['TOKEN_ADD', 'TOKEN_SUB', 'TOKEN_MUL', 'TOKEN_DIV', 'TOKEN_EQT', 'TOKEN_NEQ', 'TOKEN_ST', 'TOKEN_GT', 'TOKEN_SE', 'TOKEN_GE'].includes(peek().type)) {
      const op = peek();
      index++;
      const right = parsePrimary();
      left = {
        type: 'AST_BINARY_EXPR',
        left,
        operator: op.value,
        right
      };
    }
    
    return left;
  }

  function parsePrimary() {
    const t = peek();
    if (!t) throw new Error("Syntax Error: Unexpected EOF in expression");

    if (t.type === 'TOKEN_INT' || t.type === 'TOKEN_FLOAT' || t.type === 'TOKEN_STRING' || t.type === 'TOKEN_BOOL' || t.type === 'TOKEN_NULL') {
      index++;
      return { type: 'AST_LITERAL_EXPR', value: t.value, valueType: t.type };
    }

    if (t.type === 'TOKEN_IDENTIFIER') {
      index++;
      // check if it's a function call or just reference
      if (peek() && peek().type === 'TOKEN_LPARETH') {
        index++; // consume '('
        consume('TOKEN_RPARETH', "Expected ')' after function call argument list");
        return { type: 'AST_CALL_FXN', name: t.value };
      }
      return { type: 'AST_VAR_REF', name: t.value };
    }

    if (t.type === 'TOKEN_LPARETH') {
      index++;
      const expr = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')' after expression");
      return expr;
    }

    throw new Error(`Syntax Error: Unexpected token '${t.value}' in expression on line ${t.line}`);
  }

  function parseStatement() {
    const t = peek();
    if (!t) return null;

    if (t.type === 'TOKEN_VAR') {
      index++;
      const id = consume('TOKEN_IDENTIFIER', "Expected identifier after 'var'");
      consume('TOKEN_ASSIGN', "Expected '=' after variable identifier");
      const value = parseExpression();
      consume('TOKEN_SEMICOLON', "Expected ';' after variable declaration");
      return {
        type: 'AST_VAR_DECL',
        name: id.value,
        value
      };
    }

    if (t.type === 'TOKEN_PRINTLN') {
      index++;
      consume('TOKEN_LPARETH', "Expected '(' after 'println'");
      const value = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')' after println expression");
      consume('TOKEN_SEMICOLON', "Expected ';' after println statement");
      return {
        type: 'AST_PRINTLN',
        value
      };
    }

    if (t.type === 'TOKEN_IDENTIFIER') {
      index++;
      if (peek() && peek().type === 'TOKEN_ASSIGN') {
        index++; // consume '='
        const value = parseExpression();
        consume('TOKEN_SEMICOLON', "Expected ';' after assignment");
        return {
          type: 'AST_VAR_ASS',
          name: t.value,
          value
        };
      }
      
      // Function call as statement
      if (peek() && peek().type === 'TOKEN_LPARETH') {
        index++; // consume '('
        consume('TOKEN_RPARETH', "Expected ')' after function call");
        consume('TOKEN_SEMICOLON', "Expected ';' after function statement");
        return {
          type: 'AST_CALL_FXN',
          name: t.value
        };
      }
      
      throw new Error(`Syntax Error: Unexpected identifier statement '${t.value}' on line ${t.line}`);
    }

    if (t.type === 'TOKEN_WHILE') {
      index++;
      consume('TOKEN_LPARETH', "Expected '(' after 'while'");
      const condition = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')' after loop condition");
      const thenBlock = parseBlock();
      return {
        type: 'AST_WHILE',
        condition,
        thenBlock
      };
    }

    if (t.type === 'TOKEN_FOR') {
      index++;
      consume('TOKEN_LPARETH', "Expected '(' after 'for'");
      // Loop variable init (simplified)
      consume('TOKEN_VAR', "Expected 'var' declaration in for loop initialization");
      const id = consume('TOKEN_IDENTIFIER', "Expected identifier in for loop");
      consume('TOKEN_ASSIGN', "Expected '=' in for loop initialization");
      const startVal = parseExpression();
      consume('TOKEN_SEMICOLON', "Expected ';' after for loop initialization");
      
      const condition = parseExpression();
      consume('TOKEN_SEMICOLON', "Expected ';' after for loop condition");
      
      // Update
      const updateId = consume('TOKEN_IDENTIFIER', "Expected update statement in for loop");
      consume('TOKEN_ASSIGN', "Expected '=' in update statement");
      const updateExpr = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')' after for loop update");
      
      const thenBlock = parseBlock();
      
      return {
        type: 'AST_FOR',
        init: { type: 'AST_VAR_DECL', name: id.value, value: startVal },
        condition,
        update: { type: 'AST_VAR_ASS', name: updateId.value, value: updateExpr },
        thenBlock
      };
    }

    if (t.type === 'TOKEN_IF') {
      index++;
      consume('TOKEN_LPARETH', "Expected '(' after 'if'");
      const condition = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')' after if condition");
      const thenBlock = parseBlock();
      let elseBlock = null;

      if (peek() && peek().type === 'TOKEN_ELSE') {
        index++;
        elseBlock = parseBlock();
      } else if (peek() && peek().type === 'TOKEN_ELIF') {
        // Simple elif parsing
        elseBlock = parseStatement();
      }

      return {
        type: 'AST_IF',
        condition,
        thenBlock,
        elseBlock
      };
    }

    if (t.type === 'TOKEN_FXN') {
      index++;
      const id = consume('TOKEN_IDENTIFIER', "Expected function name after 'fxn'");
      consume('TOKEN_LPARETH', "Expected '(' after function name");
      consume('TOKEN_RPARETH', "Expected ')' after function parameter list");
      consume('TOKEN_ARROW', "Expected '->' for return type mapping");
      consume('TOKEN_LPARETH', "Expected '(' for return type wrapper");
      const retType = consume('TOKEN_VOID', "Expected 'void' return type");
      consume('TOKEN_RPARETH', "Expected ')' after return type");
      const body = parseBlock();
      return {
        type: 'AST_FUNCTION',
        name: id.value,
        body
      };
    }

    // If it's a semicolon, skip
    if (t.type === 'TOKEN_SEMICOLON') {
      index++;
      return null;
    }

    throw new Error(`Syntax Error: Unexpected token '${t.value}' on line ${t.line}`);
  }

  function parseBlock() {
    consume('TOKEN_LBRACE', "Expected '{' to open statement block");
    const statements = [];
    while (peek() && peek().type !== 'TOKEN_RBRACE' && peek().type !== 'TOKEN_EOF') {
      const stmt = parseStatement();
      if (stmt) statements.push(stmt);
    }
    consume('TOKEN_RBRACE', "Expected '}' to close statement block");
    return {
      type: 'AST_BLOCK',
      statements
    };
  }

  const rootStatements = [];
  while (peek() && peek().type !== 'TOKEN_EOF') {
    const stmt = parseStatement();
    if (stmt) rootStatements.push(stmt);
  }

  return {
    type: 'AST_PROGRAM',
    statements: rootStatements
  };
}

// Generate Mock LLVM IR in JS
function jsCodegen(ast) {
  let output = "; --- Apollo Simulated LLVM IR Backend Output ---\n";
  output += "declare i8* @printf(i8*, ...)\n";
  output += "declare i8* @malloc(i64)\n\n";
  output += "@str_format = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n";
  output += "@int_format = private unnamed_addr constant [4 x i8] c\"%d\\0A\\00\"\n\n";

  let tempCount = 1;
  let strConstCount = 0;
  const strings = {};

  function genExpr(expr) {
    if (expr.type === 'AST_LITERAL_EXPR') {
      if (expr.valueType === 'TOKEN_STRING') {
        const val = expr.value;
        const len = val.length + 1;
        const name = `@.str.${strConstCount++}`;
        strings[name] = { val, len };
        const reg = `%t.${tempCount++}`;
        output += `  ${reg} = getelementptr inbounds [${len} x i8], [${len} x i8]* ${name}, i64 0, i64 0\n`;
        return { reg, type: 'i8*' };
      }
      if (expr.valueType === 'TOKEN_INT') {
        return { reg: expr.value, type: 'i32' };
      }
      return { reg: expr.value, type: 'i32' };
    }
    if (expr.type === 'AST_VAR_REF') {
      const reg = `%t.${tempCount++}`;
      output += `  ${reg} = load i32, i32* %${expr.name}\n`;
      return { reg, type: 'i32' };
    }
    if (expr.type === 'AST_BINARY_EXPR') {
      const left = genExpr(expr.left);
      const right = genExpr(expr.right);
      const reg = `%t.${tempCount++}`;
      const opMap = {
        '+': 'add nsw i32',
        '-': 'sub nsw i32',
        '*': 'mul nsw i32',
        '/': 'sdiv i32'
      };
      const op = opMap[expr.operator] || 'add nsw i32';
      output += `  ${reg} = ${op} ${left.type} ${left.reg}, ${right.reg}\n`;
      return { reg, type: 'i32' };
    }
    return { reg: '0', type: 'i32' };
  }

  function walk(node) {
    if (!node) return;

    if (node.type === 'AST_PROGRAM') {
      node.statements.forEach(walk);
    } else if (node.type === 'AST_FUNCTION') {
      const isMain = node.name === 'main' || node.name === 'run';
      const llvmName = isMain ? 'main' : node.name;
      output += `define i32 @${llvmName}() {\n`;
      node.body.statements.forEach(walk);
      output += "  ret i32 0\n";
      output += "}\n\n";
    } else if (node.type === 'AST_VAR_DECL') {
      output += `  %${node.name} = alloca i32\n`;
      const res = genExpr(node.value);
      output += `  store i32 ${res.reg}, i32* %${node.name}\n`;
    } else if (node.type === 'AST_VAR_ASS') {
      const res = genExpr(node.value);
      output += `  store i32 ${res.reg}, i32* %${node.name}\n`;
    } else if (node.type === 'AST_PRINTLN') {
      const res = genExpr(node.value);
      if (res.type === 'i8*') {
        output += `  call i8* (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str_format, i64 0, i64 0), i8* ${res.reg})\n`;
      } else {
        output += `  call i8* (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @int_format, i64 0, i64 0), i32 ${res.reg})\n`;
      }
    } else if (node.type === 'AST_WHILE') {
      const loopCond = `loop.cond.${tempCount++}`;
      const loopBody = `loop.body.${tempCount++}`;
      const loopEnd = `loop.end.${tempCount++}`;
      
      output += `  br label %${loopCond}\n\n`;
      output += `${loopCond}:\n`;
      const res = genExpr(node.condition);
      const cmpReg = `%t.${tempCount++}`;
      output += `  ${cmpReg} = icmp ne i32 ${res.reg}, 0\n`;
      output += `  br i1 ${cmpReg}, label %${loopBody}, label %${loopEnd}\n\n`;
      
      output += `${loopBody}:\n`;
      node.thenBlock.statements.forEach(walk);
      output += `  br label %${loopCond}\n\n`;
      
      output += `${loopEnd}:\n`;
    } else if (node.type === 'AST_CALL_FXN') {
      output += `  call i32 @${node.name}()\n`;
    }
  }

  // If no main/run function explicitly, wrap global scope in a main
  const hasFunction = ast.statements.some(s => s.type === 'AST_FUNCTION');
  if (!hasFunction) {
    output += "define i32 @main() {\n";
    ast.statements.forEach(walk);
    output += "  ret i32 0\n";
    output += "}\n\n";
  } else {
    ast.statements.forEach(walk);
  }

  // Prepend string constants at the top
  let strConsts = "";
  Object.keys(strings).forEach(name => {
    const meta = strings[name];
    strConsts += `${name} = private unnamed_addr constant [${meta.len} x i8] c\"${meta.val}\\00\"\n`;
  });
  
  if (strConsts) {
    output = output.replace("declare i8*", strConsts + "\ndeclare i8*");
  }

  return output;
}

// Simple JS execution runner for outputs
function jsSimulateRun(ast) {
  const output = [];
  const variables = {};

  function evalExpr(expr) {
    if (expr.type === 'AST_LITERAL_EXPR') {
      if (expr.valueType === 'TOKEN_BOOL') {
        return expr.value === 'true';
      }
      if (expr.valueType === 'TOKEN_INT') {
        return parseInt(expr.value, 10);
      }
      if (expr.valueType === 'TOKEN_FLOAT') {
        return parseFloat(expr.value);
      }
      return expr.value;
    }
    if (expr.type === 'AST_VAR_REF') {
      if (variables[expr.name] === undefined) {
        throw new Error(`Reference Error: '${expr.name}' is not declared.`);
      }
      return variables[expr.name];
    }
    if (expr.type === 'AST_BINARY_EXPR') {
      const left = evalExpr(expr.left);
      const right = evalExpr(expr.right);
      switch (expr.operator) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': return Math.floor(left / right);
        case '==': return left === right ? 1 : 0;
        case '!=': return left !== right ? 1 : 0;
        case '<': return left < right ? 1 : 0;
        case '>': return left > right ? 1 : 0;
        case '<=': return left <= right ? 1 : 0;
        case '>=': return left >= right ? 1 : 0;
        default: return left + right;
      }
    }
    return null;
  }

  const functions = {};
  
  function executeNode(node) {
    if (node.type === 'AST_VAR_DECL') {
      variables[node.name] = evalExpr(node.value);
    } else if (node.type === 'AST_VAR_ASS') {
      if (variables[node.name] === undefined) {
        throw new Error(`Reference Error: variable '${node.name}' was not declared before assignment.`);
      }
      variables[node.name] = evalExpr(node.value);
    } else if (node.type === 'AST_PRINTLN') {
      const val = evalExpr(node.value);
      output.push(val !== null ? val.toString() : "null");
    } else if (node.type === 'AST_WHILE') {
      let limit = 0;
      while (evalExpr(node.condition) !== 0) {
        limit++;
        if (limit > 1000) {
          throw new Error("Runtime Error: Possible infinite loop detected! Execution terminated.");
        }
        node.thenBlock.statements.forEach(executeNode);
      }
    } else if (node.type === 'AST_FOR') {
      executeNode(node.init);
      let limit = 0;
      while (evalExpr(node.condition) !== 0) {
        limit++;
        if (limit > 1000) {
          throw new Error("Runtime Error: Possible infinite loop detected! Execution terminated.");
        }
        node.thenBlock.statements.forEach(executeNode);
        executeNode(node.update);
      }
    } else if (node.type === 'AST_IF') {
      const cond = evalExpr(node.condition);
      if (cond !== 0 && cond !== false) {
        node.thenBlock.statements.forEach(executeNode);
      } else if (node.elseBlock) {
        if (node.elseBlock.type === 'AST_BLOCK') {
          node.elseBlock.statements.forEach(executeNode);
        } else {
          executeNode(node.elseBlock);
        }
      }
    } else if (node.type === 'AST_FUNCTION') {
      functions[node.name] = node;
    } else if (node.type === 'AST_CALL_FXN') {
      const fxn = functions[node.name];
      if (!fxn) {
        throw new Error(`Reference Error: Function '${node.name}' is not defined.`);
      }
      fxn.body.statements.forEach(executeNode);
    }
  }

  // Collect function headers first
  ast.statements.forEach(s => {
    if (s.type === 'AST_FUNCTION') {
      executeNode(s);
    }
  });

  // Execute main or global nodes
  const hasFunction = ast.statements.some(s => s.type === 'AST_FUNCTION');
  if (hasFunction) {
    const mainFxn = functions['main'] || functions['run'];
    if (mainFxn) {
      mainFxn.body.statements.forEach(executeNode);
    } else {
      output.push("[SIMULATOR] Error: No main() or run() function found to execute.");
    }
  } else {
    ast.statements.forEach(s => {
      if (s.type !== 'AST_FUNCTION') {
        executeNode(s);
      }
    });
  }

  return output;
}

// ==========================================
// 2. REACT MAIN APP WRAPPER
// ==========================================

export default function App() {
  const [activeTab, setActiveTab] = useState('intro');
  const [visStep, setVisStep] = useState('tokens');
  
  // Playground state
  const [codeSample, setCodeSample] = useState('hello');
  const [playgroundCode, setPlaygroundCode] = useState(
`fxn main() -> (void) {
  var welcome = "Hello, Apollo Compiler!";
  println(welcome);
  
  var score = 100;
  score = score + 45;
  println(score);
}`
  );
  const [pipelineResults, setPipelineResults] = useState(null);
  const [pipelineError, setPipelineError] = useState(null);

  const samples = {
    hello: 
`fxn main() -> (void) {
  var msg = "Welcome to Apollo Language!";
  println(msg);
}`,
    math:
`fxn main() -> (void) {
  var a = 15;
  var b = 27;
  var result = a * b + 10;
  println(result);
}`,
    loop:
`fxn main() -> (void) {
  var count = 5;
  while (count > 0) {
    println(count);
    count = count - 1;
  }
}`,
    ifElse:
`fxn main() -> (void) {
  var score = 85;
  if (score > 90) {
    println("Excellent");
  } else {
    println("Needs Improvement");
  }
}`
  };

  const handleSampleChange = (e) => {
    const key = e.target.value;
    setCodeSample(key);
    if (samples[key]) {
      setPlaygroundCode(samples[key]);
    }
  };

  const runVisualizer = () => {
    setPipelineError(null);
    try {
      const tokens = jsLex(playgroundCode);
      const ast = jsParse(tokens);
      const llvm = jsCodegen(ast);
      const termLog = jsSimulateRun(ast);
      
      setPipelineResults({
        tokens,
        ast,
        llvm,
        termLog
      });
    } catch (err) {
      setPipelineError(err.message);
      setPipelineResults(null);
    }
  };

  // Helper to render tree nodes of AST visually
  const renderASTNode = (node, index = 0) => {
    if (!node) return null;
    
    if (node.type === 'AST_PROGRAM') {
      return (
        <div key="program" className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_PROGRAM</span>
          </div>
          {node.statements.map((stmt, i) => renderASTNode(stmt, i))}
        </div>
      );
    }

    if (node.type === 'AST_FUNCTION') {
      return (
        <div key={`fxn-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_FUNCTION</span>
            <span className="ast-node-val">(name: "{node.name}")</span>
          </div>
          {renderASTNode(node.body)}
        </div>
      );
    }

    if (node.type === 'AST_BLOCK') {
      return (
        <div key={`block-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_BLOCK</span>
            <span className="ast-node-val">({node.statements.length} statements)</span>
          </div>
          {node.statements.map((stmt, i) => renderASTNode(stmt, i))}
        </div>
      );
    }

    if (node.type === 'AST_VAR_DECL') {
      return (
        <div key={`decl-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_VAR_DECL</span>
            <span className="ast-node-val">(name: "{node.name}")</span>
          </div>
          {renderASTNode(node.value)}
        </div>
      );
    }

    if (node.type === 'AST_VAR_ASS') {
      return (
        <div key={`ass-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_VAR_ASS</span>
            <span className="ast-node-val">(target: "{node.name}")</span>
          </div>
          {renderASTNode(node.value)}
        </div>
      );
    }

    if (node.type === 'AST_PRINTLN') {
      return (
        <div key={`println-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_PRINTLN</span>
          </div>
          {renderASTNode(node.value)}
        </div>
      );
    }

    if (node.type === 'AST_BINARY_EXPR') {
      return (
        <div key={`bin-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_BINARY_EXPR</span>
            <span className="ast-node-val">(op: "{node.operator}")</span>
          </div>
          {renderASTNode(node.left)}
          {renderASTNode(node.right)}
        </div>
      );
    }

    if (node.type === 'AST_LITERAL_EXPR') {
      return (
        <div key={`lit-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_LITERAL_EXPR</span>
            <span className="ast-node-val">(value: "{node.value}")</span>
          </div>
        </div>
      );
    }

    if (node.type === 'AST_VAR_REF') {
      return (
        <div key={`ref-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_VAR_REF</span>
            <span className="ast-node-val">(name: "{node.name}")</span>
          </div>
        </div>
      );
    }

    if (node.type === 'AST_WHILE') {
      return (
        <div key={`while-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_WHILE</span>
          </div>
          {renderASTNode(node.condition)}
          {renderASTNode(node.thenBlock)}
        </div>
      );
    }

    return null;
  };

  return (
    <div className="app-container">
      {/* Background glow effects */}
      <div className="bg-glow glow-orange" />
      <div className="bg-glow glow-purple" />

      {/* Sidebar Navigation */}
      <aside className="sidebar">
        <div className="sidebar-header">
          <div className="sidebar-logo">Apollo Compiler</div>
          <div className="sidebar-version">v1.0.0</div>
        </div>
        
        <nav className="sidebar-nav">
          <div className="nav-group">
            <div className="nav-group-title">Documentation</div>
            <button 
              className={`nav-link ${activeTab === 'intro' ? 'active' : ''}`}
              onClick={() => setActiveTab('intro')}
            >
              <BookOpen size={16} />
              Overview &amp; Features
            </button>
            <button 
              className={`nav-link ${activeTab === 'pipeline' ? 'active' : ''}`}
              onClick={() => setActiveTab('pipeline')}
            >
              <Layers size={16} />
              Compiler Pipeline
            </button>
          </div>

          <div className="nav-group">
            <div className="nav-group-title">Compiler Engine</div>
            <button 
              className={`nav-link ${activeTab === 'lexer' ? 'active' : ''}`}
              onClick={() => setActiveTab('lexer')}
            >
              <Code size={16} />
              The Lexer (C)
            </button>
            <button 
              className={`nav-link ${activeTab === 'parser' ? 'active' : ''}`}
              onClick={() => setActiveTab('parser')}
            >
              <Cpu size={16} />
              Parser &amp; AST Construction
            </button>
            <button 
              className={`nav-link ${activeTab === 'generator' ? 'active' : ''}`}
              onClick={() => setActiveTab('generator')}
            >
              <Settings size={16} />
              LLVM IR Code Gen
            </button>
            <button 
              className={`nav-link ${activeTab === 'errors' ? 'active' : ''}`}
              onClick={() => setActiveTab('errors')}
            >
              <AlertCircle size={16} />
              Panic-Mode Error Recovery
            </button>
          </div>

          <div className="nav-group">
            <div className="nav-group-title">Interactive Tooling</div>
            <button 
              className={`nav-link ${activeTab === 'playground' ? 'active' : ''}`}
              onClick={() => setActiveTab('playground')}
            >
              <Terminal size={16} />
              Pipeline Visualizer
            </button>
          </div>
        </nav>

        <div className="sidebar-footer">
          <span>Backend: C / LLVM</span>
          <span>Target: Windows</span>
        </div>
      </aside>

      {/* Main Content Area */}
      <main className="main-content">
        
        {/* tab: OVERVIEW */}
        {activeTab === 'intro' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">Apollo Programming Language</h1>
              <div className="docs-description">A lightweight, high-performance compiler written in C targeting LLVM IR.</div>
            </div>

            <section>
              <p>
                Apollo is a compact, custom programming language designed to demonstrate clean, modern compiler architecture. The compiler compiler driver compiles <code>.apl</code> files into LLVM intermediate representation (IR), which it then optimizes and links using Clang into native Windows executables.
              </p>

              <div className="callout callout-info">
                <div className="callout-icon">
                  <BookOpen size={20} style={{ color: 'var(--color-purple-primary)' }} />
                </div>
                <div className="callout-content">
                  <h4 className="callout-title">Core Implementation Goal</h4>
                  <p className="callout-text">
                    Apollo is engineered to show how hand-rolled lexical scanners, AST parsers with robust error recovery, and LLVM emission integrate to produce production-grade binary compilation.
                  </p>
                </div>
              </div>

              <h2 className="section-title">Key Language Capabilities</h2>
              <div className="docs-table-wrapper">
                <table className="docs-table">
                  <thead>
                    <tr>
                      <th>Feature</th>
                      <th>Syntax Example</th>
                      <th>C Handler</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td><strong>Variables</strong></td>
                      <td><code>var score = 100;</code></td>
                      <td><code>variables.c</code></td>
                    </tr>
                    <tr>
                      <td><strong>Data Types</strong></td>
                      <td>Integers, Floats, Strings, Booleans, Nulls</td>
                      <td><code>ast.c</code> / <code>defs.h</code></td>
                    </tr>
                    <tr>
                      <td><strong>Arithmetic Expressions</strong></td>
                      <td><code>+</code>, <code>-</code>, <code>*</code>, <code>/</code>, <code>%</code></td>
                      <td><code>arithmetic.h</code></td>
                    </tr>
                    <tr>
                      <td><strong>String Concatenation</strong></td>
                      <td><code>var msg = "Score: " + score;</code></td>
                      <td><code>generator.c</code> (dynamic strings via snprintf)</td>
                    </tr>
                    <tr>
                      <td><strong>Control Flow</strong></td>
                      <td><code>if</code>, <code>elif</code>, <code>else</code></td>
                      <td><code>src/generator.c</code> (emit comparison &amp; conditional branch)</td>
                    </tr>
                    <tr>
                      <td><strong>Looping Structures</strong></td>
                      <td><code>while (cond) &#123;...&#125;</code> and <code>for</code> loops</td>
                      <td><code>src/generator.c</code></td>
                    </tr>
                    <tr>
                      <td><strong>Functions</strong></td>
                      <td><code>fxn name() -&gt; (void) &#123;...&#125;</code></td>
                      <td><code>src/generator.c</code> (generates LLVM function definitions)</td>
                    </tr>
                  </tbody>
                </table>
              </div>

              <h2 className="section-title">Natural Steps For Extension</h2>
              <p>
                Currently, Apollo features a flat global symbol table structure and function execution without parameters/arguments. 
                Next natural development tasks include adding local lexical scoping levels, struct/array support, and dedicated command-line compilation options.
              </p>
            </section>
          </div>
        )}

        {/* tab: PIPELINE */}
        {activeTab === 'pipeline' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">Compiler Compilation Pipeline</h1>
              <div className="docs-description">How Apollo source code is converted to optimized machine instructions.</div>
            </div>

            <section>
              <p>
                The Apollo compiler operates as a single-pass compiler that parses source code directly into an Abstract Syntax Tree (AST) while building intermediate symbol definitions. It then traverses this AST to generate standard LLVM Assembly language.
              </p>

              {/* Pipeline Flow Visual */}
              <div className="pipeline-flow">
                <div className="pipeline-node active">
                  <div className="pipeline-node-title">Source File</div>
                  <div className="pipeline-node-desc">main.apl</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Lexer</div>
                  <div className="pipeline-node-desc">lexer.c</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Parser</div>
                  <div className="pipeline-node-desc">parser.c</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">AST</div>
                  <div className="pipeline-node-desc">ast.c</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Generator</div>
                  <div className="pipeline-node-desc">generator.c</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node active" style={{ borderColor: 'var(--color-purple-primary)' }}>
                  <div className="pipeline-node-title">Clang Backend</div>
                  <div className="pipeline-node-desc">program.exe</div>
                </div>
              </div>

              <h2 className="section-title">The Five Compiler Steps</h2>
              <ol style={{ paddingLeft: '1.5rem', color: 'var(--text-secondary)' }}>
                <li style={{ marginBottom: '1rem' }}>
                  <strong style={{ color: 'var(--text-primary)' }}>Source Intake:</strong> 
                  The compiler driver reads raw text from the specified file stream.
                </li>
                <li style={{ marginBottom: '1rem' }}>
                  <strong style={{ color: 'var(--text-primary)' }}>Lexical Analysis:</strong> 
                  Characters are scanned and clustered into semantic packages called <code>Tokens</code> (keywords, identifiers, literals, operators).
                </li>
                <li style={{ marginBottom: '1rem' }}>
                  <strong style={{ color: 'var(--text-primary)' }}>Syntax Parsing:</strong> 
                  The parser evaluates the token sequence against grammar rules. If a token violates expectations, Panic-Mode Error recovery is engaged to synchronize states at statements boundaries.
                </li>
                <li style={{ marginBottom: '1rem' }}>
                  <strong style={{ color: 'var(--text-primary)' }}>LLVM Code Generation:</strong> 
                  Once a clean AST is parsed, the code generator emits compliant LLVM Intermediate Representation (IR).
                </li>
                <li style={{ marginBottom: '1rem' }}>
                  <strong style={{ color: 'var(--text-primary)' }}>Native Optimization &amp; Linking:</strong> 
                  Clang compiling processes compile the LLVM IR using the <code>-O3</code> flags into optimized Windows machine code.
                </li>
              </ol>
            </section>
          </div>
        )}

        {/* tab: LEXER */}
        {activeTab === 'lexer' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">The Lexical Scanner</h1>
              <div className="docs-description">Hand-crafted scanner that breaks character streams into syntactic tokens.</div>
            </div>

            <section>
              <p>
                Apollo avoids using external lexer engines like Flex. The entire lexer is hand-coded in <code>src/lexer.c</code>. It matches string patterns, classifies them into types declared in <code>headers/token.h</code>, and keeps track of character coordinates.
              </p>

              <h2 className="section-title">Token Classification</h2>
              <p>
                Tokens in Apollo hold metadata specifying the type, start memory address inside the source, length of bytes, and the line number:
              </p>

              <div className="code-block-wrapper">
                <div className="code-block-header">
                  <div className="code-block-title">
                    <FileText size={14} />
                    headers/token.h (Excerpt)
                  </div>
                </div>
                <pre className="code-block-content">
                  <code>
{`typedef struct {
  TokenType type;    // Classification ID
  const char *start; // Raw pointer to characters
  int length;        // Byte length
  int line;          // File line number
} Token;`}
                  </code>
                </pre>
              </div>

              <h2 className="section-title">Scanning Algorithm</h2>
              <p>
                The lexical scanning engine consumes characters sequentially. It uses utility routines like <code>isspace()</code>, <code>isdigit()</code>, and <code>isalpha()</code>. If it detects a lexical anomaly (e.g. an unterminated string block), it pushes a diagnostic description onto the compiler's global <code>errorStack</code> instead of hard-crashing.
              </p>
            </section>
          </div>
        )}

        {/* tab: PARSER */}
        {activeTab === 'parser' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">Syntax Parser &amp; AST Construction</h1>
              <div className="docs-description">Building the abstract structure of the Apollo program.</div>
            </div>

            <section>
              <p>
                The Parser (<code>src/parser.c</code>) validates grammar using recursive descent. It generates AST nodes representing the program structure, statements, loops, variables, and expressions.
              </p>

              <h2 className="section-title">AST Node Representation</h2>
              <p>
                Every syntactic element in Apollo is modeled as a polymorphic <code>ASTNode</code>. It contains an enum field <code>Type</code> and a large union containing structs for each statement class:
              </p>

              <div className="code-block-wrapper">
                <div className="code-block-header">
                  <div className="code-block-title">
                    <FileText size={14} />
                    headers/ast.h (Excerpt)
                  </div>
                </div>
                <pre className="code-block-content">
                  <code>
{`struct ASTNode {
  ASTNodeType Type;
  union {
    struct {
      const char *name;
      datatype value_type;
      ASTNode *value;
    } var_decl;
    struct {
      ASTNode *left;
      TokenType operator_type;
      ASTNode *right;
    } binary_expr;
    struct {
      ASTNode *condition;
      ASTNode *then_block;
      ASTNode *else_block;
    } if_stmt;
    // ...
  };
};`}
                  </code>
                </pre>
              </div>

              <h2 className="section-title">Grammar Parser Loop</h2>
              <p>
                The parser loops through statements until it hits <code>TOKEN_EOF</code>. Inside blocks, statements are accumulated into arrays. If compilation encounters syntax errors, code generation is skipped.
              </p>
            </section>
          </div>
        )}

        {/* tab: GENERATOR */}
        {activeTab === 'generator' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">LLVM IR Code Generation</h1>
              <div className="docs-description">Translating the abstract tree structure into platform-independent assembly.</div>
            </div>

            <section>
              <p>
                The code generator (<code>src/generator.c</code>) compiles AST nodes to optimized LLVM Intermediate Representation (IR).
              </p>

              <h2 className="section-title">LLVM Output Initialization</h2>
              <p>
                When code generation starts, Apollo prints the standard environment configurations. It declares references to necessary external C runtime library functions:
              </p>

              <div className="code-block-wrapper">
                <div className="code-block-header">
                  <div className="code-block-title">
                    <FileText size={14} />
                    src/generator.c (LLVM Setup)
                  </div>
                </div>
                <pre className="code-block-content">
                  <code>
{`declare i8* @printf(i8*, ...)
declare i8* @malloc(i64)
declare i8* @strcat(i8*, i8*)
declare i8* @strcpy(i8*, i8*)
declare i32 @strcmp(i8*, i8*)
declare i32 @snprintf(i8*, i64, i8*, ...)`}
                  </code>
                </pre>
              </div>

              <h2 className="section-title">Generating String Concatenation</h2>
              <p>
                A key feature of the Apollo compiler is dynamic string concatenation (using the <code>+</code> operator). The code generator generates an dynamic allocation call via <code>malloc</code>, then invokes <code>snprintf</code> to merge string data or automatically format numbers to strings.
              </p>
            </section>
          </div>
        )}

        {/* tab: ERRORS */}
        {activeTab === 'errors' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">Panic-Mode Error Recovery</h1>
              <div className="docs-description">How Apollo avoids cascading phantom errors during parsing.</div>
            </div>

            <section>
              <p>
                If a compiler encounters a syntax error and keeps trying to parse blindly, it can trigger a huge cascade of hundreds of fake errors. Apollo prevents this using a technique called <strong>Panic-Mode Error Recovery</strong>.
              </p>

              <div className="callout callout-warning">
                <div className="callout-icon">
                  <AlertCircle size={20} style={{ color: 'var(--text-warning)' }} />
                </div>
                <div className="callout-content">
                  <h4 className="callout-title">The Synchronization Process</h4>
                  <p className="callout-text">
                    When the parser hits an unexpected token, it reports the error and calls a <code>synchronize()</code> routine. This routine skips tokens until it finds a statement boundary (like a semicolon <code>;</code> or starting keywords like <code>var</code>, <code>fxn</code>, <code>if</code>, <code>while</code>).
                  </p>
                </div>
              </div>

              <h2 className="section-title">Formatting Error Contexts</h2>
              <p>
                All syntax errors are accumulated on the global <code>errorStack</code> and printed at the end of execution. They display the exact line number, token content, and character coordinates:
              </p>

              <div className="code-block-wrapper">
                <div className="code-block-header">
                  <div className="code-block-title">
                    <FileText size={14} />
                    Syntax Error Sample Output
                  </div>
                </div>
                <pre className="code-block-content">
                  <code>
{`[PARSER ERROR] Syntax Error: Expected '=' after variable identifier on line 4
    Found: "println"
    Context: synchronize() engaged, looking for next statement.`}
                  </code>
                </pre>
              </div>
            </section>
          </div>
        )}

        {/* tab: PLAYGROUND */}
        {activeTab === 'playground' && (
          <div>
            <div className="docs-header">
              <h1 className="docs-title">Interactive Pipeline Visualizer</h1>
              <div className="docs-description">Write Apollo code and watch it compile step-by-step through the pipeline.</div>
            </div>

            <section>
              <p>
                Choose one of the presets or type your own code in the editor panel below. Press <strong>Run Compiler Pipeline</strong> to simulate the lexer, parser, AST generation, and LLVM code emission!
              </p>

              <div className="playground-layout">
                {/* Left Panel: Editor */}
                <div className="panel">
                  <div className="panel-header">
                    <span className="panel-title">
                      <Code size={16} style={{ color: 'var(--color-orange-primary)' }} />
                      Apollo Editor
                    </span>
                    <select 
                      className="playground-select" 
                      value={codeSample}
                      onChange={handleSampleChange}
                    >
                      <option value="hello">Sample: Hello World</option>
                      <option value="math">Sample: Math &amp; Vars</option>
                      <option value="loop">Sample: While Loops</option>
                      <option value="ifElse">Sample: Conditionals</option>
                    </select>
                  </div>
                  <textarea 
                    className="editor-textarea" 
                    value={playgroundCode}
                    onChange={(e) => setPlaygroundCode(e.target.value)}
                  />
                  <div style={{ padding: '1rem', borderTop: '1px solid var(--color-border)', display: 'flex', justifyContent: 'flex-end' }}>
                    <button className="btn-compile" onClick={runVisualizer}>
                      <Play size={14} />
                      Run Compiler Pipeline
                    </button>
                  </div>
                </div>

                {/* Right Panel: Visualization */}
                <div className="panel">
                  <div className="panel-header">
                    <span className="panel-title">
                      <RefreshCw size={14} style={{ color: 'var(--color-purple-primary)' }} />
                      Pipeline Steps
                    </span>
                  </div>
                  
                  {pipelineError && (
                    <div style={{ padding: '1.5rem', color: 'var(--text-error)', display: 'flex', gap: '0.75rem' }}>
                      <AlertCircle size={20} style={{ flexShrink: 0 }} />
                      <div>
                        <h4 style={{ margin: '0 0 0.25rem 0', fontWeight: 600 }}>Compiler Error Detected</h4>
                        <p style={{ margin: 0, fontSize: '0.85rem', color: 'rgba(239, 68, 68, 0.8)' }}>{pipelineError}</p>
                      </div>
                    </div>
                  )}

                  {!pipelineResults && !pipelineError && (
                    <div className="vis-content">
                      <div className="pipeline-status">
                        <Terminal size={40} style={{ color: 'var(--text-muted)' }} />
                        <span>Ready. Click "Run Compiler Pipeline" to compile.</span>
                      </div>
                    </div>
                  )}

                  {pipelineResults && (
                    <React.Fragment>
                      <div className="vis-steps">
                        <button 
                          className={`vis-step-btn ${visStep === 'tokens' ? 'active' : ''}`}
                          onClick={() => setVisStep('tokens')}
                        >
                          1. Tokens
                        </button>
                        <button 
                          className={`vis-step-btn ${visStep === 'ast' ? 'active' : ''}`}
                          onClick={() => setVisStep('ast')}
                        >
                          2. AST
                        </button>
                        <button 
                          className={`vis-step-btn ${visStep === 'llvm' ? 'active' : ''}`}
                          onClick={() => setVisStep('llvm')}
                        >
                          3. LLVM IR
                        </button>
                        <button 
                          className={`vis-step-btn ${visStep === 'output' ? 'active' : ''}`}
                          onClick={() => setVisStep('output')}
                        >
                          4. Binary Run
                        </button>
                      </div>

                      <div className="vis-content">
                        {visStep === 'tokens' && (
                          <div className="token-grid">
                            {pipelineResults.tokens.map((tok, idx) => (
                              <div key={idx} className="token-card">
                                <span className="token-type">{tok.type}</span>
                                <span className="token-value">{tok.value}</span>
                                <span className="token-line">Line {tok.line}</span>
                              </div>
                            ))}
                          </div>
                        )}

                        {visStep === 'ast' && (
                          <div style={{ textAlign: 'left' }}>
                            {renderASTNode(pipelineResults.ast)}
                          </div>
                        )}

                        {visStep === 'llvm' && (
                          <pre style={{ margin: 0, textAlign: 'left', overflowX: 'auto', whiteSpace: 'pre-wrap' }}>
                            <code>{pipelineResults.llvm}</code>
                          </pre>
                        )}

                        {visStep === 'output' && (
                          <div>
                            <div style={{ color: 'var(--text-secondary)', marginBottom: '0.5rem' }}>Simulated execution of linked Windows binary:</div>
                            <div className="terminal-output">
                              {pipelineResults.termLog.map((line, idx) => (
                                <div key={idx} className="terminal-line">{line}</div>
                              ))}
                              <div style={{ color: 'var(--text-muted)', marginTop: '1rem', borderTop: '1px solid rgba(255,255,255,0.05)', paddingTop: '0.5rem', fontSize: '0.75rem' }}>
                                Process exited with code 0.
                              </div>
                            </div>
                          </div>
                        )}
                      </div>
                    </React.Fragment>
                  )}
                </div>
              </div>
            </section>
          </div>
        )}

      </main>
    </div>
  );
}
