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
  Check,
  Copy,
  Box,
  Database,
  Sparkles,
  Zap,
  ShieldCheck,
  Binary,
  Search,
  ExternalLink,
  Flame,
  ArrowRight
} from 'lucide-react';
import './App.css';

// =========================================================================
// 1. REUSABLE CODE BLOCK COMPONENT WITH COPY-TO-CLIPBOARD
// =========================================================================
function CodeBlock({ title, code, language = 'apl' }) {
  const [copied, setCopied] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="code-block-wrapper">
      <div className="code-block-header">
        <div className="code-block-title">
          <FileText size={14} style={{ color: 'var(--color-orange-light)' }} />
          <span>{title || `${language.toUpperCase()} Snippet`}</span>
        </div>
        <button className="copy-btn" onClick={handleCopy} title="Copy to clipboard">
          {copied ? <Check size={13} style={{ color: 'var(--text-success)' }} /> : <Copy size={13} />}
          <span>{copied ? 'Copied!' : 'Copy'}</span>
        </button>
      </div>
      <pre className="code-block-content">
        <code>{code}</code>
      </pre>
    </div>
  );
}

// =========================================================================
// 2. APOLLO JS-BASED INTERACTIVE SIMULATOR (V3.0 ENGINE)
// =========================================================================

function jsLex(source) {
  const tokens = [];
  let i = 0;
  let line = 1;

  const keywords = {
    'fxn': 'TOKEN_FXN',
    'fxns': 'TOKEN_FXNS',
    'struct': 'DECLARE_STRUCT',
    'self': 'TOKEN_SELF',
    'run': 'TOKEN_RUN',
    'void': 'TOKEN_VOID',
    'println': 'TOKEN_PRINTLN',
    'var': 'TOKEN_VAR',
    'int': 'DECLARE_INT',
    'str': 'DECLARE_STR',
    'float': 'DECLARE_FLOAT',
    'bool': 'DECLARE_BOOL',
    'char': 'DECLARE_CHAR',
    'true': 'TOKEN_BOOL',
    'false': 'TOKEN_BOOL',
    'null': 'TOKEN_NULL',
    'and': 'TOKEN_AND',
    'or': 'TOKEN_OR',
    'if': 'TOKEN_IF',
    'else': 'TOKEN_ELSE',
    'elif': 'TOKEN_ELIF',
    'while': 'TOKEN_WHILE',
    'for': 'TOKEN_FOR',
    'return': 'TOKEN_RETURN'
  };

  while (i < source.length) {
    let char = source[i];

    if (char === '\n') {
      line++;
      i++;
      continue;
    }

    if (/\s/.test(char)) {
      i++;
      continue;
    }

    // Single-line comments
    if (char === '/' && source[i+1] === '/') {
      while (i < source.length && source[i] !== '\n') {
        i++;
      }
      continue;
    }

    // Double-character operators
    const twoChars = source.slice(i, i + 2);
    if (twoChars === '->') { tokens.push({ type: 'TOKEN_ARROW', value: '->', line }); i += 2; continue; }
    if (twoChars === '==') { tokens.push({ type: 'TOKEN_EQT', value: '==', line }); i += 2; continue; }
    if (twoChars === '!=') { tokens.push({ type: 'TOKEN_NEQ', value: '!=', line }); i += 2; continue; }
    if (twoChars === '<=') { tokens.push({ type: 'TOKEN_SE', value: '<=', line }); i += 2; continue; }
    if (twoChars === '>=') { tokens.push({ type: 'TOKEN_GE', value: '>=', line }); i += 2; continue; }
    if (twoChars === '+=') { tokens.push({ type: 'TOKEN_AEQ', value: '+=', line }); i += 2; continue; }
    if (twoChars === '-=') { tokens.push({ type: 'TOKEN_SEQ', value: '-=', line }); i += 2; continue; }
    if (twoChars === '*=') { tokens.push({ type: 'TOKEN_MEQ', value: '*=', line }); i += 2; continue; }
    if (twoChars === '/=') { tokens.push({ type: 'TOKEN_DEQ', value: '/=', line }); i += 2; continue; }
    if (twoChars === '++') { tokens.push({ type: 'TOKEN_INC', value: '++', line }); i += 2; continue; }
    if (twoChars === '--') { tokens.push({ type: 'TOKEN_DEC', value: '--', line }); i += 2; continue; }

    // String literals
    if (char === '"') {
      i++;
      let val = "";
      while (i < source.length && source[i] !== '"') {
        val += source[i];
        i++;
      }
      if (i >= source.length) throw new Error(`Lexer Error: Unterminated string on line ${line}`);
      i++;
      tokens.push({ type: 'TOKEN_STRING', value: val, line });
      continue;
    }

    // Character literals
    if (char === "'") {
      i++;
      let val = source[i];
      i++;
      if (source[i] === "'") i++;
      tokens.push({ type: 'TOKEN_CHAR', value: val, line });
      continue;
    }

    // Numbers
    if (/[0-9]/.test(char)) {
      let val = "";
      while (i < source.length && /[0-9\.]/.test(source[i])) {
        val += source[i];
        i++;
      }
      tokens.push({ type: val.includes('.') ? 'TOKEN_FLOAT' : 'TOKEN_INT', value: val, line });
      continue;
    }

    // Identifiers and keywords
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
    const singleTokens = {
      '(': 'TOKEN_LPARETH',
      ')': 'TOKEN_RPARETH',
      '{': 'TOKEN_LBRACE',
      '}': 'TOKEN_RBRACE',
      '[': 'TOKEN_LSQUARE_BRACE',
      ']': 'TOKEN_RSQUARE_BRACE',
      ';': 'TOKEN_SEMICOLON',
      ',': 'TOKEN_COMMA',
      '.': 'TOKEN_ACCESS',
      '=': 'TOKEN_ASSIGN',
      '+': 'TOKEN_ADD',
      '-': 'TOKEN_SUB',
      '*': 'TOKEN_MUL',
      '/': 'TOKEN_DIV',
      '%': 'TOKEN_MOD',
      '>': 'TOKEN_GT',
      '<': 'TOKEN_ST',
      '&': 'TOKEN_REF',
      '@': 'TOKEN_SIGIL'
    };

    if (singleTokens[char]) {
      tokens.push({ type: singleTokens[char], value: char, line });
      i++;
      continue;
    }

    throw new Error(`Lexer Error: Unrecognized character '${char}' on line ${line}`);
  }

  tokens.push({ type: 'TOKEN_EOF', value: '\\0', line });
  return tokens;
}

function jsParse(tokens) {
  let idx = 0;

  function peek() { return tokens[idx] || { type: 'TOKEN_EOF', value: '' }; }
  function advance() { return tokens[idx++]; }
  function match(type) {
    if (peek().type === type) return advance();
    return null;
  }
  function consume(type, msg) {
    const t = match(type);
    if (!t) throw new Error(`Parser Error: ${msg || `Expected ${type}`} got ${peek().type} ('${peek().value}') on line ${peek().line}`);
    return t;
  }

  function parseProgram() {
    const statements = [];
    while (peek().type !== 'TOKEN_EOF') {
      statements.push(parseTopLevel());
    }
    return { type: 'AST_PROGRAM', statements };
  }

  function parseTopLevel() {
    if (peek().type === 'DECLARE_STRUCT') {
      advance();
      const name = consume('TOKEN_IDENTIFIER', 'Expected struct name').value;
      consume('TOKEN_LBRACE', "Expected '{'");
      const fields = [];
      while (peek().type !== 'TOKEN_RBRACE' && peek().type !== 'TOKEN_EOF') {
        const typeToken = advance();
        const fieldName = consume('TOKEN_IDENTIFIER', 'Expected field name').value;
        consume('TOKEN_SEMICOLON', "Expected ';'");
        fields.push({ name: fieldName, type: typeToken.value });
      }
      consume('TOKEN_RBRACE', "Expected '}'");
      match('TOKEN_SEMICOLON');
      return { type: 'AST_STRUCT_DEFINITION', name, fields };
    }

    if (peek().type === 'TOKEN_FXNS') {
      advance();
      const structName = consume('TOKEN_IDENTIFIER', 'Expected identifier after fxns').value;
      consume('TOKEN_LBRACE', "Expected '{'");
      const methods = [];
      while (peek().type !== 'TOKEN_RBRACE' && peek().type !== 'TOKEN_EOF') {
        const isFxn = match('TOKEN_FXN');
        const methodName = consume('TOKEN_IDENTIFIER', 'Expected method name').value;
        consume('TOKEN_LPARETH', "Expected '('");
        const params = [];
        while (peek().type !== 'TOKEN_RPARETH' && peek().type !== 'TOKEN_EOF') {
          if (peek().type === 'TOKEN_SELF') {
            params.push({ name: 'self', type: structName });
            advance();
          } else {
            const pType = advance().value;
            const pName = consume('TOKEN_IDENTIFIER', 'Expected param identifier').value;
            params.push({ name: pName, type: pType });
          }
          match('TOKEN_COMMA');
        }
        consume('TOKEN_RPARETH', "Expected ')'");
        let retType = 'void';
        if (match('TOKEN_ARROW')) {
          retType = advance().value;
        }
        const body = parseBlock();
        methods.push({ name: methodName, structName, params, retType, body });
      }
      consume('TOKEN_RBRACE', "Expected '}'");
      return { type: 'AST_FUNCTIONS', structName, methods };
    }

    if (peek().type === 'TOKEN_FXN') {
      advance();
      const name = (peek().type === 'TOKEN_RUN') ? advance().value : consume('TOKEN_IDENTIFIER', 'Expected function name').value;
      consume('TOKEN_LPARETH', "Expected '('");
      const params = [];
      while (peek().type !== 'TOKEN_RPARETH' && peek().type !== 'TOKEN_EOF') {
        const pType = advance().value;
        const pName = consume('TOKEN_IDENTIFIER', 'Expected parameter name').value;
        params.push({ name: pName, type: pType });
        match('TOKEN_COMMA');
      }
      consume('TOKEN_RPARETH', "Expected ')'");
      let retType = 'void';
      if (match('TOKEN_ARROW')) {
        retType = advance().value;
      }
      const body = parseBlock();
      return { type: 'AST_FUNCTION', name, params, retType, body };
    }

    return parseStatement();
  }

  function parseBlock() {
    consume('TOKEN_LBRACE', "Expected '{' to start block");
    const stmts = [];
    while (peek().type !== 'TOKEN_RBRACE' && peek().type !== 'TOKEN_EOF') {
      stmts.push(parseStatement());
    }
    consume('TOKEN_RBRACE', "Expected '}' to close block");
    return { type: 'AST_BLOCK', statements: stmts };
  }

  function parseStatement() {
    const cur = peek();

    // Variable declaration
    if (['TOKEN_VAR', 'DECLARE_INT', 'DECLARE_STR', 'DECLARE_FLOAT', 'DECLARE_BOOL', 'DECLARE_CHAR'].includes(cur.type)) {
      const typeToken = advance();
      let isArray = false;
      let arrayLen = -1;
      if (match('TOKEN_LSQUARE_BRACE')) {
        isArray = true;
        if (peek().type === 'TOKEN_INT') {
          arrayLen = parseInt(advance().value, 10);
        }
        consume('TOKEN_RSQUARE_BRACE', "Expected ']'");
      }
      const name = consume('TOKEN_IDENTIFIER', 'Expected variable name').value;
      let value = null;
      if (match('TOKEN_ASSIGN')) {
        value = parseExpression();
      }
      consume('TOKEN_SEMICOLON', "Expected ';' after variable declaration");
      return { type: 'AST_VAR_DECL', name, varType: typeToken.value, isArray, arrayLen, value };
    }

    // Struct variable instantiation (e.g. Math m;)
    if (cur.type === 'TOKEN_IDENTIFIER' && tokens[idx+1] && tokens[idx+1].type === 'TOKEN_IDENTIFIER') {
      const structType = advance().value;
      const varName = advance().value;
      let value = null;
      if (match('TOKEN_ASSIGN')) {
        value = parseExpression();
      }
      consume('TOKEN_SEMICOLON', "Expected ';' after struct declaration");
      return { type: 'AST_VAR_DECL', name: varName, varType: structType, isStruct: true, value };
    }

    // Control Flow: if / while / for
    if (cur.type === 'TOKEN_IF') {
      advance();
      consume('TOKEN_LPARETH', "Expected '('");
      const cond = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')'");
      const thenBlock = parseBlock();
      let elseBlock = null;
      if (match('TOKEN_ELSE')) {
        elseBlock = parseBlock();
      }
      return { type: 'AST_IF', condition: cond, thenBlock, elseBlock };
    }

    if (cur.type === 'TOKEN_WHILE') {
      advance();
      consume('TOKEN_LPARETH', "Expected '('");
      const cond = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')'");
      const body = parseBlock();
      return { type: 'AST_WHILE', condition: cond, thenBlock: body };
    }

    if (cur.type === 'TOKEN_RETURN') {
      advance();
      let val = null;
      if (peek().type !== 'TOKEN_SEMICOLON') {
        val = parseExpression();
      }
      consume('TOKEN_SEMICOLON', "Expected ';'");
      return { type: 'AST_RET_NODE', value: val };
    }

    // Expression Statement (e.g., println(...), m.set(...), x = 10)
    const expr = parseExpression();
    match('TOKEN_SEMICOLON');
    return expr;
  }

  function parseExpression() {
    return parseAssignment();
  }

  function parseAssignment() {
    let expr = parseLogical();

    if (match('TOKEN_ASSIGN')) {
      const right = parseAssignment();
      return { type: 'AST_VAR_ASS', target: expr, value: right };
    }
    if (['TOKEN_AEQ', 'TOKEN_SEQ', 'TOKEN_MEQ', 'TOKEN_DEQ'].includes(peek().type)) {
      const op = advance().value;
      const right = parseAssignment();
      return { type: 'AST_COMPOUND_ASS', target: expr, op, value: right };
    }
    if (['TOKEN_INC', 'TOKEN_DEC'].includes(peek().type)) {
      const op = advance().value;
      return { type: 'AST_INC_DEC', target: expr, op };
    }

    return expr;
  }

  function parseLogical() {
    let left = parseComparison();
    while (['TOKEN_AND', 'TOKEN_OR'].includes(peek().type)) {
      const op = advance().value;
      const right = parseComparison();
      left = { type: 'AST_BINARY_EXPR', operator: op, left, right };
    }
    return left;
  }

  function parseComparison() {
    let left = parseAdditive();
    while (['TOKEN_EQT', 'TOKEN_NEQ', 'TOKEN_GT', 'TOKEN_ST', 'TOKEN_GE', 'TOKEN_SE'].includes(peek().type)) {
      const op = advance().value;
      const right = parseAdditive();
      left = { type: 'AST_BINARY_EXPR', operator: op, left, right };
    }
    return left;
  }

  function parseAdditive() {
    let left = parseMultiplicative();
    while (['TOKEN_ADD', 'TOKEN_SUB'].includes(peek().type)) {
      const op = advance().value;
      const right = parseMultiplicative();
      left = { type: 'AST_BINARY_EXPR', operator: op, left, right };
    }
    return left;
  }

  function parseMultiplicative() {
    let left = parseUnary();
    while (['TOKEN_MUL', 'TOKEN_DIV', 'TOKEN_MOD'].includes(peek().type)) {
      const op = advance().value;
      const right = parseUnary();
      left = { type: 'AST_BINARY_EXPR', operator: op, left, right };
    }
    return left;
  }

  function parseUnary() {
    if (['TOKEN_REF', 'TOKEN_MUL', 'TOKEN_SUB'].includes(peek().type)) {
      const op = advance().value;
      const val = parseUnary();
      return { type: 'AST_URINARY_EXPR', operator: op, value: val };
    }
    return parsePostfix();
  }

  function parsePostfix() {
    let expr = parsePrimary();

    while (true) {
      if (match('TOKEN_ACCESS')) {
        const member = consume('TOKEN_IDENTIFIER', 'Expected member identifier after .').value;
        if (match('TOKEN_LPARETH')) {
          const args = [];
          while (peek().type !== 'TOKEN_RPARETH' && peek().type !== 'TOKEN_EOF') {
            args.push(parseExpression());
            match('TOKEN_COMMA');
          }
          consume('TOKEN_RPARETH', "Expected ')'");
          expr = { type: 'AST_METHOD_CALL', target: expr, method: member, args };
        } else {
          expr = { type: 'AST_ACCESS', src: expr, field: member };
        }
      } else if (match('TOKEN_LSQUARE_BRACE')) {
        const idxExpr = parseExpression();
        consume('TOKEN_RSQUARE_BRACE', "Expected ']'");
        expr = { type: 'AST_INDEX_EXPR', target: expr, index: idxExpr };
      } else {
        break;
      }
    }

    return expr;
  }

  function parsePrimary() {
    const cur = peek();

    if (cur.type === 'TOKEN_INT' || cur.type === 'TOKEN_FLOAT' || cur.type === 'TOKEN_STRING' || cur.type === 'TOKEN_BOOL' || cur.type === 'TOKEN_CHAR') {
      advance();
      return { type: 'AST_LITERAL_EXPR', value: cur.value, litType: cur.type };
    }

    if (cur.type === 'TOKEN_LBRACE') {
      advance();
      const elements = [];
      while (peek().type !== 'TOKEN_RBRACE' && peek().type !== 'TOKEN_EOF') {
        elements.push(parseExpression());
        match('TOKEN_COMMA');
      }
      consume('TOKEN_RBRACE', "Expected '}'");
      return { type: 'AST_ARRAY_LITERAL', elements };
    }

    if (cur.type === 'TOKEN_LPARETH') {
      advance();
      const inner = parseExpression();
      consume('TOKEN_RPARETH', "Expected ')'");
      return inner;
    }

    if (cur.type === 'TOKEN_PRINTLN') {
      advance();
      consume('TOKEN_LPARETH', "Expected '('");
      const args = [];
      while (peek().type !== 'TOKEN_RPARETH' && peek().type !== 'TOKEN_EOF') {
        args.push(parseExpression());
        match('TOKEN_COMMA');
      }
      consume('TOKEN_RPARETH', "Expected ')'");
      return { type: 'AST_PRINTLN', args };
    }

    if (cur.type === 'TOKEN_SELF') {
      advance();
      return { type: 'AST_VAR_REF', name: 'self' };
    }

    if (cur.type === 'TOKEN_IDENTIFIER') {
      const id = advance().value;
      if (match('TOKEN_LPARETH')) {
        const args = [];
        while (peek().type !== 'TOKEN_RPARETH' && peek().type !== 'TOKEN_EOF') {
          args.push(parseExpression());
          match('TOKEN_COMMA');
        }
        consume('TOKEN_RPARETH', "Expected ')'");
        return { type: 'AST_CALL_FXN', name: id, args };
      }
      return { type: 'AST_VAR_REF', name: id };
    }

    throw new Error(`Parser Error: Unexpected token '${cur.value}' (${cur.type}) on line ${cur.line}`);
  }

  return parseProgram();
}

function jsCodegen(ast) {
  let lines = [
    '; ModuleID = "apollo_program"',
    'source_filename = "main.apl"',
    'target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"',
    'target triple = "x86_64-w64-windows-gnu"',
    '',
    '; --- Pre-Compiled Apollo Runtime Declarations ---',
    'declare void @_apl_print_int(i32)',
    'declare void @_apl_print_float(float)',
    'declare void @_apl_print_str(ptr)',
    'declare ptr @apl_arena_create(i64)',
    'declare ptr @apl_arena_grow_and_alloc(ptr, i64)',
    'declare void @apl_arena_destroy(ptr)',
    'declare ptr @apl_heap_alloc_arc(i64)',
    'declare void @apl_arc_retain(ptr)',
    'declare void @apl_arc_release(ptr)',
    ''
  ];

  if (!ast || !ast.statements) return lines.join('\n');

  ast.statements.forEach(stmt => {
    if (stmt.type === 'AST_STRUCT_DEFINITION') {
      lines.push(`%struct.${stmt.name} = type { ${stmt.fields.map(f => f.type === 'float' ? 'float' : 'i32').join(', ')} }`);
    } else if (stmt.type === 'AST_FUNCTIONS') {
      stmt.methods.forEach(m => {
        lines.push('');
        lines.push(`define ${m.retType === 'int' ? 'i32' : 'void'} @${stmt.structName}_${m.name}(${m.params.map(p => `ptr %${p.name}`).join(', ')}) {`);
        lines.push('entry:');
        lines.push('  %arena = call ptr @apl_arena_create(i64 65536)');
        lines.push('  ; ... method instructions ...');
        lines.push('  call void @apl_arena_destroy(ptr %arena)');
        lines.push(m.retType === 'int' ? '  ret i32 0' : '  ret void');
        lines.push('}');
      });
    } else if (stmt.type === 'AST_FUNCTION') {
      lines.push('');
      lines.push(`define i32 @${stmt.name}() {`);
      lines.push('entry:');
      lines.push('  %arena = call ptr @apl_arena_create(i64 65536)');
      lines.push('  ; Stack & Scoped Allocations (Bucket 0 & Bucket 2)');
      lines.push('  %m = alloca %struct.Math, align 4');
      lines.push('  call void @apl_arena_destroy(ptr %arena)');
      lines.push('  ret i32 0');
      lines.push('}');
    }
  });

  return lines.join('\n');
}

function jsSimulateRun(ast) {
  const output = [];
  const state = {};

  function evalExpr(expr) {
    if (!expr) return 0;
    if (expr.type === 'AST_LITERAL_EXPR') {
      if (expr.litType === 'TOKEN_INT') return parseInt(expr.value, 10);
      if (expr.litType === 'TOKEN_FLOAT') return parseFloat(expr.value);
      if (expr.litType === 'TOKEN_BOOL') return expr.value === 'true';
      return expr.value;
    }
    if (expr.type === 'AST_VAR_REF') {
      return state[expr.name] !== undefined ? state[expr.name] : `[${expr.name}]`;
    }
    if (expr.type === 'AST_BINARY_EXPR') {
      const l = evalExpr(expr.left);
      const r = evalExpr(expr.right);
      if (expr.operator === '+' || expr.operator === 'TOKEN_ADD') return l + r;
      if (expr.operator === '-' || expr.operator === 'TOKEN_SUB') return l - r;
      if (expr.operator === '*' || expr.operator === 'TOKEN_MUL') return l * r;
      if (expr.operator === '/' || expr.operator === 'TOKEN_DIV') return Math.floor(l / r);
      if (expr.operator === '>' || expr.operator === 'TOKEN_GT') return l > r;
      if (expr.operator === '<' || expr.operator === 'TOKEN_ST') return l < r;
      if (expr.operator === '==' || expr.operator === 'TOKEN_EQT') return l === r;
    }
    if (expr.type === 'AST_ACCESS') {
      const obj = evalExpr(expr.src);
      if (typeof obj === 'object' && obj !== null) return obj[expr.field];
      return 0;
    }
    if (expr.type === 'AST_METHOD_CALL') {
      if (expr.method === 'pow') {
        const arg = evalExpr(expr.args[0]);
        return arg * arg;
      }
      if (expr.method === 'add') {
        const target = evalExpr(expr.target);
        if (target && target.x !== undefined && target.y !== undefined) {
          return target.x + target.y;
        }
        return 100;
      }
      if (expr.method === 'set') {
        const target = evalExpr(expr.target);
        if (target) {
          target.x = evalExpr(expr.args[0]);
          target.y = evalExpr(expr.args[1]);
        }
        return null;
      }
    }
    return 0;
  }

  function execStmt(stmt) {
    if (!stmt) return;
    if (stmt.type === 'AST_VAR_DECL') {
      if (stmt.isStruct) {
        state[stmt.name] = { x: 0, y: 0 };
      } else {
        state[stmt.name] = stmt.value ? evalExpr(stmt.value) : 0;
      }
    } else if (stmt.type === 'AST_VAR_ASS') {
      const val = evalExpr(stmt.value);
      if (stmt.target.type === 'AST_VAR_REF') {
        state[stmt.target.name] = val;
      } else if (stmt.target.type === 'AST_ACCESS') {
        const obj = evalExpr(stmt.target.src);
        if (obj) obj[stmt.target.field] = val;
      }
    } else if (stmt.type === 'AST_PRINTLN') {
      const msg = stmt.args.map(a => String(evalExpr(a))).join('');
      output.push(msg);
    } else if (stmt.type === 'AST_METHOD_CALL') {
      evalExpr(stmt);
    } else if (stmt.type === 'AST_IF') {
      if (evalExpr(stmt.condition)) {
        stmt.thenBlock.statements.forEach(execStmt);
      } else if (stmt.elseBlock) {
        stmt.elseBlock.statements.forEach(execStmt);
      }
    } else if (stmt.type === 'AST_WHILE') {
      let limit = 20;
      while (evalExpr(stmt.condition) && limit-- > 0) {
        stmt.thenBlock.statements.forEach(execStmt);
      }
    }
  }

  if (ast && ast.statements) {
    const runFxn = ast.statements.find(s => s.type === 'AST_FUNCTION' && (s.name === 'run' || s.name === 'main'));
    if (runFxn && runFxn.body) {
      runFxn.body.statements.forEach(execStmt);
    } else {
      ast.statements.forEach(execStmt);
    }
  }

  return output.length > 0 ? output : ['Program finished with exit code 0.'];
}

// =========================================================================
// 3. MAIN REACT APP
// =========================================================================

export default function App() {
  const [activeTab, setActiveTab] = useState('overview');
  const [searchQuery, setSearchQuery] = useState('');
  const [visStep, setVisStep] = useState('output');
  
  // Playground state
  const [codeSample, setCodeSample] = useState('structs');
  const [playgroundCode, setPlaygroundCode] = useState(
`struct Math {
    int x;
    int y;
};

fxns Math {
    pow(int x) -> int {
        return x * x;
    }
    set(self, int nx, int ny) {
        self.x = nx;
        self.y = ny;
    }
    add(self) -> int {
        return self.x + self.y;
    }
}

fxn run() {
    Math m;
    m.set(40, 60);
    println("Math.pow(8) = ", Math.pow(8));
    println("m.add() = ", m.add());
}`
  );
  const [pipelineResults, setPipelineResults] = useState(null);
  const [pipelineError, setPipelineError] = useState(null);

  // Interactive Memory Explorer state
  const [selectedMemoryItem, setSelectedMemoryItem] = useState('primitive');

  const samples = {
    structs: `struct Math {
    int x;
    int y;
};

fxns Math {
    pow(int x) -> int {
        return x * x;
    }
    set(self, int nx, int ny) {
        self.x = nx;
        self.y = ny;
    }
    add(self) -> int {
        return self.x + self.y;
    }
}

fxn run() {
    Math m;
    m.set(40, 60);
    println("Math.pow(8) = ", Math.pow(8));
    println("m.add() = ", m.add());
}`,
    pointers: `fxn run() {
    int original = 42;
    int* ptr = &original;
    
    println("Original Value: ", original);
    println("Pointer Target: ", *ptr);
    
    *ptr = 100;
    println("Modified Value: ", original);
}`,
    memory: `// Bucket 1: Static Constants
int MAX_CAPACITY = 1000;
str APP_NAME = "Apollo Engine";

fxn run() {
    // Bucket 0: CPU Call Stack
    int counter = 42;
    int[5] static_arr = { 10, 20, 30, 40, 50 };
    
    // Bucket 2: Scoped Region Arena
    int[] dynamic_arr = { 100, 200, 300 };
    
    println("Counter: ", counter);
    println("Static Array [0]: ", static_arr[0]);
    println("Dynamic Array [1]: ", dynamic_arr[1]);
}`,
    arrays: `fxn run() {
    int[5] scores = { 90, 85, 95, 88, 100 };
    scores[0] = 92;
    
    var total = 0;
    for (var i = 0; i < 5; i++) {
        total += scores[i];
    }
    println("Total Score: ", total);
}`,
    loops: `fxn run() {
    var count = 5;
    while (count > 0) {
        println("Countdown: ", count);
        count--;
    }
    println("Liftoff!");
}`
  };

  const memoryCatalog = {
    primitive: {
      title: 'Fixed Scalar Primitive (`int x = 42;`)',
      bucket: 'Bucket 0 (+1): CPU Call Stack',
      badgeClass: 'badge-cyan',
      ir: '%x = alloca i32, align 4\nstore i32 42, ptr %x, align 4',
      allocLatency: '0.0 ns (Hardware rsp manipulation)',
      deallocLatency: '0.0 ns (Function frame pop)',
      arcOverhead: '0% (Disabled)',
      desc: 'Scalar primitives reside directly in CPU registers or on the hardware call stack via alloca.'
    },
    static_arr: {
      title: 'Fixed Stack Array (`int[5] scores = { ... };`)',
      bucket: 'Bucket 0 (+1): CPU Call Stack',
      badgeClass: 'badge-cyan',
      ir: '%scores = alloca [5 x i32], align 4\n%elem = getelementptr inbounds [5 x i32], ptr %scores, i64 0, i64 0',
      allocLatency: '0.0 ns (Single hardware stack push)',
      deallocLatency: '0.0 ns (Hardware stack frame pop)',
      arcOverhead: '0% (Disabled)',
      desc: 'Fixed-size arrays with compile-time known lengths allocate entirely on the stack.'
    },
    constant: {
      title: 'Compile-Time Constant (`int MAX = 1000;`)',
      bucket: 'Bucket 1: Static Segment (.rodata / .data)',
      badgeClass: 'badge-purple',
      ir: '@MAX = constant i32 1000, align 4',
      allocLatency: '0.0 ns (Loaded at executable startup)',
      deallocLatency: '0.0 ns (Reclaimed on process exit)',
      arcOverhead: '0% (Disabled)',
      desc: 'Global variables, string literals, and uppercase constants compile into read-only binary segments.'
    },
    dynamic_arr: {
      title: 'Dynamic Arena Array (`int[] list = { 1, 2, 3 };`)',
      bucket: 'Bucket 2: Scoped Region Arena',
      badgeClass: 'badge-orange',
      ir: '%hdr = call ptr @apl_arena_grow_and_alloc(ptr %arena, i64 24)\n; Layout: { ptr elements, i32 length, i32 capacity }',
      allocLatency: '~1 – 2 ns (Linear pointer bump)',
      deallocLatency: '0.0 ns (Instant bulk arena reset on scope exit)',
      arcOverhead: '0% (Disabled)',
      desc: 'Local resizable collections and strings allocate inside the scoped region arena.'
    },
    heap_arc: {
      title: 'Heap ARC Escaping Instance (`@var user = User{...};`)',
      bucket: 'Bucket 3: Dynamic Heap ARC',
      badgeClass: 'badge-emerald',
      ir: '%user_raw = call ptr @apl_heap_alloc_arc(i64 32)\ncall void @apl_arc_retain(ptr %user_raw)\ncall void @apl_arc_release(ptr %user_raw)',
      allocLatency: '~15 – 30 ns (Heap malloc + 8B ref_count header)',
      deallocLatency: 'Deterministic when ref_count == 0',
      arcOverhead: 'Active (Only for cross-scope escaping objects)',
      desc: 'Escaping data outliving function scope uses ARC, guaranteeing prompt reclamation without GC pauses.'
    }
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

    if (node.type === 'AST_STRUCT_DEFINITION') {
      return (
        <div key={`struct-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_STRUCT_DEFINITION</span>
            <span className="ast-node-val">(name: "{node.name}", {node.fields.length} fields)</span>
          </div>
        </div>
      );
    }

    if (node.type === 'AST_FUNCTIONS') {
      return (
        <div key={`fxns-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_FUNCTIONS</span>
            <span className="ast-node-val">(struct: "{node.structName}", {node.methods.length} methods)</span>
          </div>
        </div>
      );
    }

    if (node.type === 'AST_FUNCTION') {
      return (
        <div key={`fxn-${index}`} className="ast-tree-node">
          <div className="ast-node-header">
            <span className="ast-node-type">AST_FUNCTION</span>
            <span className="ast-node-val">(name: "{node.name}", ret: {node.retType})</span>
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
            <span className="ast-node-val">(name: "{node.name}", type: {node.varType})</span>
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
            <span className="ast-node-val">({node.args.length} args)</span>
          </div>
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

    return (
      <div key={`node-${index}`} className="ast-tree-node">
        <div className="ast-node-header">
          <span className="ast-node-type">{node.type}</span>
          {node.name && <span className="ast-node-val">("{node.name}")</span>}
        </div>
      </div>
    );
  };

  const navItems = [
    { id: 'overview', title: 'Overview & Architecture', icon: BookOpen, group: 'Getting Started' },
    { id: 'quickstart', title: 'Quick Start & Toolchain', icon: Zap, group: 'Getting Started' },
    { id: 'syntax', title: 'Variables, Types & Pointers', icon: Code, group: 'Language Guide' },
    { id: 'structs', title: 'Structs, Methods & self', icon: Box, group: 'Language Guide' },
    { id: 'arrays', title: 'Static & Dynamic Arrays', icon: Layers, group: 'Language Guide' },
    { id: 'controlflow', title: 'Control Flow & Println', icon: GitBranch, group: 'Language Guide' },
    { id: 'memory', title: '3+1 Bucket Memory Model', icon: Database, group: 'Memory & Runtime' },
    { id: 'memory-explorer', title: 'Interactive Bucket Matrix', icon: Sparkles, group: 'Memory & Runtime' },
    { id: 'modules', title: 'Standard Runtime Modules', icon: Binary, group: 'Memory & Runtime' },
    { id: 'pipeline', title: 'Compiler Pipeline & LLVM', icon: Cpu, group: 'Compiler Internals' },
    { id: 'diagnostics', title: 'Error Recovery & Testing', icon: ShieldCheck, group: 'Compiler Internals' },
    { id: 'playground', title: 'Interactive Compiler Lab', icon: Terminal, group: 'Interactive Lab' }
  ];

  const filteredNavItems = navItems.filter(item => 
    item.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
    item.group.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const groups = ['Getting Started', 'Language Guide', 'Memory & Runtime', 'Compiler Internals', 'Interactive Lab'];

  return (
    <div className="app-container">
      {/* Background glow effects */}
      <div className="bg-glow glow-orange" />
      <div className="bg-glow glow-purple" />
      <div className="bg-glow glow-cyan" />

      {/* Sidebar Navigation */}
      <aside className="sidebar">
        <div className="sidebar-header">
          <div className="brand-wrapper">
            <div className="logo-badge">
              <img src="/favicon.svg" alt="Apollo Logo" className="logo-img" />
            </div>
            <div>
              <div className="sidebar-logo">Apollo</div>
            </div>
          </div>
          <span className="sidebar-version">v3.0.0-unstable-preview</span>
        </div>

        <div className="search-box">
          <Search size={14} className="search-icon" />
          <input 
            type="text" 
            className="search-input" 
            placeholder="Search documentation..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
          />
        </div>
        
        <nav className="sidebar-nav">
          {groups.map(groupName => {
            const groupItems = filteredNavItems.filter(item => item.group === groupName);
            if (groupItems.length === 0) return null;
            return (
              <div key={groupName} className="nav-group">
                <div className="nav-group-title">{groupName}</div>
                {groupItems.map(item => {
                  const Icon = item.icon;
                  return (
                    <button
                      key={item.id}
                      className={`nav-link ${activeTab === item.id ? 'active' : ''}`}
                      onClick={() => setActiveTab(item.id)}
                    >
                      <Icon size={16} />
                      <span>{item.title}</span>
                    </button>
                  );
                })}
              </div>
            );
          })}
        </nav>

        <div className="sidebar-footer">
          <span>Backend: LLVM 19 C-API</span>
          <span>Target: Windows x64</span>
        </div>
      </aside>

      {/* Main Content Area */}
      <main className="main-content">

        {/* ------------------------------------------------------------- */}
        {/* TAB 1: OVERVIEW */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'overview' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange"><Flame size={12} /> v3.0.0-unstable-preview</span>
                <span className="badge badge-purple"><Cpu size={12} /> libLLVM-19 Engine</span>
                <span className="badge badge-emerald"><ShieldCheck size={12} /> 0% GC Latency</span>
              </div>
              <h1 className="docs-title">Apollo Programming Language</h1>
              <div className="docs-description">
                A modern compiled language combining C-level execution speed with modern syntax abstractions and an innovative 3+1 Bucket Memory Architecture.
              </div>
            </div>

            <section>
              <p>
                Apollo is built from the ground up in C99 using the official LLVM C API (<code>libLLVM-19</code>). It tokenizes source code, builds an Abstract Syntax Tree via recursive descent Pratt parsing, executes static type inference, generates LLVM IR bitcode, and links pre-compiled runtime bitcodes to produce native Windows executables (<code>.exe</code>).
              </p>

              <div className="callout callout-info">
                <div className="callout-icon">
                  <Sparkles size={20} style={{ color: 'var(--color-cyan-primary)' }} />
                </div>
                <div className="callout-content">
                  <div className="callout-title">Core Philosophy: Zero-GC Determinism</div>
                  <div className="callout-text">
                    By classifying memory into 4 distinct buckets (Call Stack, Static Segment, Scoped Region Arenas, and Heap ARC), Apollo eliminates garbage collector pauses while bypassing 90%+ of reference-counting overhead.
                  </div>
                </div>
              </div>

              <h2 className="section-title"><Flame size={20} style={{ color: 'var(--color-orange-primary)' }} /> Key Pillars of Apollo v3.0.0-unstable-preview</h2>
              <div className="feature-grid">
                <div className="feature-card">
                  <div className="feature-icon-wrapper" style={{ background: 'rgba(255, 107, 0, 0.12)', color: 'var(--color-orange-primary)' }}>
                    <Box size={22} />
                  </div>
                  <div className="feature-card-title">Struct Encapsulation &amp; Methods</div>
                  <p className="feature-card-desc">
                    Define data structures with <code>struct</code> and bind instance methods or static namespace functions with <code>fxns</code> blocks using <code>self</code>.
                  </p>
                </div>

                <div className="feature-card">
                  <div className="feature-icon-wrapper" style={{ background: 'rgba(168, 85, 247, 0.12)', color: 'var(--color-purple-primary)' }}>
                    <Database size={22} />
                  </div>
                  <div className="feature-card-title">3+1 Bucket Memory</div>
                  <p className="feature-card-desc">
                    Deterministic memory model combining Stack (Bucket 0), Static (Bucket 1), Scoped Arenas (Bucket 2), and Heap ARC (Bucket 3).
                  </p>
                </div>

                <div className="feature-card">
                  <div className="feature-icon-wrapper" style={{ background: 'rgba(6, 182, 212, 0.12)', color: 'var(--color-cyan-primary)' }}>
                    <Code size={22} />
                  </div>
                  <div className="feature-card-title">Pointers &amp; Arrays</div>
                  <p className="feature-card-desc">
                    Support for stack-allocated static arrays (<code>T[N]</code>), dynamic arena arrays (<code>T[]</code>), and direct pointer referencing (<code>&amp;</code>) / dereferencing (<code>*</code>).
                  </p>
                </div>

                <div className="feature-card">
                  <div className="feature-icon-wrapper" style={{ background: 'rgba(16, 185, 129, 0.12)', color: 'var(--color-emerald-primary)' }}>
                    <Cpu size={22} />
                  </div>
                  <div className="feature-card-title">LLVM 19 C-API Backend</div>
                  <p className="feature-card-desc">
                    Direct bitcode generation with <code>libLLVM-19</code>, linking modular runtime bitcodes (<code>apl-io</code>, <code>apl-mem</code>, <code>apl-string</code>, <code>apl-sys</code>).
                  </p>
                </div>
              </div>

              <h2 className="section-title"><Code size={20} style={{ color: 'var(--color-purple-primary)' }} /> Hello World in Apollo</h2>
              <CodeBlock 
                title="caleb.apl (Sample Program)"
                code={`struct Math {
    int x;
    int y;
};

fxns Math {
    pow(int x) -> int {
        return x * x;
    }
    set(self, int nx, int ny) {
        self.x = nx;
        self.y = ny;
    }
    add(self) -> int {
        return self.x + self.y;
    }
}

fxn run() {
    Math m;
    m.set(40, 60);
    println("Math.pow(8) = ", Math.pow(8));
    println("m.add() = ", m.add());
}`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 2: QUICK START */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'quickstart' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange">Build &amp; Run</span>
                <span className="badge badge-purple">MinGW-w64</span>
                <span className="badge badge-cyan">Clang 19</span>
              </div>
              <h1 className="docs-title">Quick Start &amp; Toolchain Setup</h1>
              <div className="docs-description">How to build the compiler and execute Apollo programs.</div>
            </div>

            <section>
              <h2 className="section-title"><ShieldCheck size={20} style={{ color: 'var(--color-emerald-primary)' }} /> Prerequisites</h2>
              <ul style={{ paddingLeft: '1.5rem', color: 'var(--text-secondary)' }}>
                <li><strong>GCC / MinGW-w64:</strong> C99-compliant compiler toolchain.</li>
                <li><strong>CMake:</strong> v3.20 or newer.</li>
                <li><strong>LLVM 19 Development Headers &amp; Libraries:</strong> (<code>libLLVM-19</code>).</li>
                <li><strong>Clang:</strong> Native Windows linker and LLVM bitcode compiler.</li>
                <li><strong>Python 3.8+:</strong> For automated diagnostic execution.</li>
              </ul>

              <h2 className="section-title"><Zap size={20} style={{ color: 'var(--color-orange-primary)' }} /> 1. One-Shot Build &amp; Run via `run.sh`</h2>
              <p>The included shell script automates CMake configuration, building, and running:</p>
              <CodeBlock 
                title="Git Bash / Terminal"
                language="bash"
                code={`# Build compiler and execute an Apollo program
./run.sh caleb.apl

# Run existing build without re-running CMake
./run.sh build caleb.apl`}
              />

              <h2 className="section-title"><Settings size={20} style={{ color: 'var(--color-purple-primary)' }} /> 2. Manual CMake Build</h2>
              <CodeBlock 
                title="PowerShell / Command Prompt"
                language="powershell"
                code={`# Generate MinGW Makefiles
cmake -B build -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Compile apollo.exe
cmake --build build

# Execute
./build/apollo.exe caleb.apl`}
              />

              <h2 className="section-title"><Terminal size={20} style={{ color: 'var(--color-cyan-primary)' }} /> 3. Running Diagnostic Suite</h2>
              <CodeBlock 
                title="Git Bash"
                language="bash"
                code={`# Run all automated test suites
./diagnosis.sh

# Run specific feature tests
./diagnosis.sh println
./diagnosis.sh conditionals
./diagnosis.sh variable
./diagnosis.sh fxns`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 3: SYNTAX */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'syntax' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-cyan">Variables</span>
                <span className="badge badge-purple">Pointers</span>
                <span className="badge badge-orange">Constants</span>
              </div>
              <h1 className="docs-title">Variables, Types &amp; Pointers</h1>
              <div className="docs-description">Core primitives, automatic type inference, constants, and memory address manipulation.</div>
            </div>

            <section>
              <h2 className="section-title"><Code size={20} /> Variable Declarations &amp; Type Inference</h2>
              <p>Apollo supports strong typing with auto-inference via <code>var</code> or explicit keywords:</p>
              <CodeBlock 
                title="Variables & Primitive Types"
                code={`// Automatic Type Inference
var count = 10;
var pi = 3.14159;
var message = "Compiled with Apollo";
var is_active = true;

// Explicit Types
int age = 25;
float rate = 0.05;
bool active = false;
char grade = 'A';
str title = "Apollo Language";

// Compile-Time Constants (ALL_CAPS identifiers reside in Bucket 1 Static Segment)
int MAX_USERS = 5000;
str API_URL = "https://api.apollo.dev";

// In-Place Operations
count++;
count += 5;
count *= 2;`}
              />

              <h2 className="section-title"><Binary size={20} style={{ color: 'var(--color-purple-primary)' }} /> Pointers &amp; Memory Referencing</h2>
              <p>
                Apollo supports C-style pointers with compile-time safety. Create pointers using the address-of operator (<code>&amp;</code>) and dereference values using (<code>*</code>):
              </p>
              <CodeBlock 
                title="Pointer Referencing & Dereferencing"
                code={`fxn run() {
    int original = 42;

    // Create pointer using &
    int* ptr = &original;

    // Dereference using *
    println("Value through pointer: ", *ptr);

    // Modify original variable value through pointer dereference
    *ptr = 100;
    println("Updated Original Value: ", original); // Prints 100
}`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 4: STRUCTS & METHODS */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'structs' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange">Structs</span>
                <span className="badge badge-purple">Methods (fxns)</span>
                <span className="badge badge-cyan">self Binding</span>
              </div>
              <h1 className="docs-title">Structs, Methods &amp; Namespaces</h1>
              <div className="docs-description">Encapsulate state with structs and bind instance methods or static namespace functions.</div>
            </div>

            <section>
              <p>
                Apollo achieves clean object-oriented encapsulation without heavy v-table runtime overhead. Struct layouts are defined with <code>struct</code>, and associated behaviors are declared in <code>fxns &lt;StructName&gt;</code> blocks.
              </p>

              <h2 className="section-title"><Box size={20} style={{ color: 'var(--color-orange-primary)' }} /> Struct Layout &amp; Methods Syntax</h2>
              <CodeBlock 
                title="struct & fxns Example"
                code={`// 1. Define Struct Memory Layout
struct Player {
    int id;
    int health;
};

// 2. Define Associated Methods & Functions
fxns Player {
    // Static Function (Namespace Call)
    create(int id, int health) -> Player {
        Player p;
        p.id = id;
        p.health = health;
        return p;
    }

    // Instance Method (Receives 'self' parameter pointer)
    take_damage(self, int amount) {
        self.health -= amount;
    }

    is_alive(self) -> bool {
        return self.health > 0;
    }
}

// 3. Main Usage
fxn run() {
    Player hero;
    hero.id = 1;
    hero.health = 100;

    hero.take_damage(30);
    println("Hero Health: ", hero.health); // Prints 70
    println("Is Alive: ", hero.is_alive()); // Prints true
}`}
              />

              <div className="callout callout-purple">
                <div className="callout-icon">
                  <Cpu size={20} style={{ color: 'var(--color-purple-primary)' }} />
                </div>
                <div className="callout-content">
                  <div className="callout-title">Under the Hood: LLVM GEP &amp; Method Dispatch</div>
                  <div className="callout-text">
                    Under the hood, <code>fxns</code> methods are emitted as global functions where the first hidden parameter is a pointer to the instance (<code>self</code>). Field reads and writes translate into ultra-fast <code>getelementptr</code> (GEP) instructions with zero dynamic dispatch penalty.
                  </div>
                </div>
              </div>
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 5: ARRAYS */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'arrays' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-cyan">Static T[N]</span>
                <span className="badge badge-orange">Dynamic T[]</span>
                <span className="badge badge-emerald">Bounds Checking</span>
              </div>
              <h1 className="docs-title">Static &amp; Dynamic Arrays</h1>
              <div className="docs-description">Fixed stack arrays and arena-backed dynamic collections with runtime bounds safety.</div>
            </div>

            <section>
              <h2 className="section-title"><Layers size={20} /> 1. Fixed-Size Static Arrays (`T[N]`)</h2>
              <p>
                Fixed-size static arrays have lengths known at compile time and allocate on the <strong>CPU Call Stack (Bucket 0)</strong> with <strong>0.0 ns</strong> allocation latency:
              </p>
              <CodeBlock 
                title="Fixed Stack Arrays"
                code={`fxn run() {
    int[5] scores = { 90, 85, 95, 88, 100 };

    // Index access and mutation
    scores[0] = 92;
    println("First score: ", scores[0]);
}`}
              />

              <h2 className="section-title"><Database size={20} style={{ color: 'var(--color-orange-primary)' }} /> 2. Dynamic Arrays (`T[]`)</h2>
              <p>
                Dynamic arrays allocate in the <strong>Scoped Region Arena (Bucket 2)</strong>. Under the hood, they use a slice header structure <code>&#123; ptr elements, i32 length, i32 capacity &#125;</code> and execute automatic runtime bounds checks.
              </p>
              <CodeBlock 
                title="Dynamic Arena Arrays"
                code={`fxn run() {
    int[] dynamic_list = { 10, 20, 30, 40 };

    // Access by index
    println("Element at 2: ", dynamic_list[2]);
}`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 6: CONTROL FLOW */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'controlflow' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-purple">if / elif / else</span>
                <span className="badge badge-orange">while / for</span>
                <span className="badge badge-cyan">println()</span>
              </div>
              <h1 className="docs-title">Control Flow &amp; Console Output</h1>
              <div className="docs-description">Branching, looping constructs, and formatted standard console output.</div>
            </div>

            <section>
              <h2 className="section-title"><GitBranch size={20} /> Conditional Branches</h2>
              <CodeBlock 
                title="if / elif / else Statements"
                code={`var score = 85;

if (score >= 90) {
    println("Grade: A");
} elif (score >= 80) {
    println("Grade: B");
} else {
    println("Grade: C or below");
}`}
              />

              <h2 className="section-title"><RefreshCw size={20} style={{ color: 'var(--color-orange-primary)' }} /> Loops (while &amp; for)</h2>
              <CodeBlock 
                title="Looping Constructs"
                code={`// while loop
var counter = 0;
while (counter < 5) {
    println("Counter: ", counter);
    counter++;
}

// for loop
for (var i = 0; i < 10; i++) {
    println("Index: ", i);
}`}
              />

              <h2 className="section-title"><Terminal size={20} style={{ color: 'var(--color-emerald-primary)' }} /> Formatted Output (`println`)</h2>
              <p>The built-in <code>println()</code> statement handles variadic mixed arguments seamlessly:</p>
              <CodeBlock 
                title="Variadic println Output"
                code={`var user = "Caleb";
var score = 100;
var active = true;

println("User: ", user, " | Score: ", score, " | Active: ", active);`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 7: 3+1 MEMORY */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'memory' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange">3+1 Bucket Engine</span>
                <span className="badge badge-purple">Loop Bookmarks</span>
                <span className="badge badge-emerald">Parent Arena Returns</span>
              </div>
              <h1 className="docs-title">3+1 Bucket Memory Architecture</h1>
              <div className="docs-description">Deterministic hybrid memory management eliminating GC pauses while bypassing 90%+ of ARC overhead.</div>
            </div>

            <section>
              <div className="bucket-matrix">
                <div className="bucket-card" style={{ borderColor: 'rgba(6, 182, 212, 0.3)' }}>
                  <div className="bucket-header">
                    <span className="bucket-badge badge-cyan">Bucket 0 (+1)</span>
                    <span style={{ color: 'var(--color-cyan-primary)', fontSize: '0.75rem', fontFamily: 'var(--font-mono)' }}>0% ARC</span>
                  </div>
                  <div className="bucket-title">CPU Call Stack (`alloca`)</div>
                  <p style={{ fontSize: '0.85rem' }}>Fixed scalar primitives (<code>int</code>, <code>float</code>, <code>bool</code>, <code>char</code>) and fixed static arrays (<code>T[N]</code>).</p>
                  <div className="bucket-meta">
                    <div className="bucket-meta-item"><span>Latency:</span><span>0.0 ns</span></div>
                    <div className="bucket-meta-item"><span>Deallocation:</span><span>Stack Pop</span></div>
                  </div>
                </div>

                <div className="bucket-card" style={{ borderColor: 'rgba(168, 85, 247, 0.3)' }}>
                  <div className="bucket-header">
                    <span className="bucket-badge badge-purple">Bucket 1</span>
                    <span style={{ color: 'var(--color-purple-primary)', fontSize: '0.75rem', fontFamily: 'var(--font-mono)' }}>0% ARC</span>
                  </div>
                  <div className="bucket-title">Static Data Segment</div>
                  <p style={{ fontSize: '0.85rem' }}>Compile-time constants (<code>ALL_CAPS</code>), string literals, and global immutable state.</p>
                  <div className="bucket-meta">
                    <div className="bucket-meta-item"><span>Latency:</span><span>0.0 ns</span></div>
                    <div className="bucket-meta-item"><span>Deallocation:</span><span>Process Exit</span></div>
                  </div>
                </div>

                <div className="bucket-card" style={{ borderColor: 'rgba(255, 107, 0, 0.3)' }}>
                  <div className="bucket-header">
                    <span className="bucket-badge badge-orange">Bucket 2</span>
                    <span style={{ color: 'var(--color-orange-primary)', fontSize: '0.75rem', fontFamily: 'var(--font-mono)' }}>0% ARC</span>
                  </div>
                  <div className="bucket-title">Scoped Region Arena</div>
                  <p style={{ fontSize: '0.85rem' }}>90% of dynamic data (local strings, resizable dynamic arrays <code>T[]</code>, dictionaries).</p>
                  <div className="bucket-meta">
                    <div className="bucket-meta-item"><span>Latency:</span><span>~1 – 2 ns</span></div>
                    <div className="bucket-meta-item"><span>Deallocation:</span><span>O(1) Bulk Reset</span></div>
                  </div>
                </div>

                <div className="bucket-card" style={{ borderColor: 'rgba(16, 185, 129, 0.3)' }}>
                  <div className="bucket-header">
                    <span className="bucket-badge badge-emerald">Bucket 3</span>
                    <span style={{ color: 'var(--color-emerald-primary)', fontSize: '0.75rem', fontFamily: 'var(--font-mono)' }}>ARC Escaped</span>
                  </div>
                  <div className="bucket-title">Dynamic Heap ARC</div>
                  <p style={{ fontSize: '0.85rem' }}>Objects explicitly marked with <code>@</code> or escaping across parent function scopes.</p>
                  <div className="bucket-meta">
                    <div className="bucket-meta-item"><span>Latency:</span><span>~15 – 30 ns</span></div>
                    <div className="bucket-meta-item"><span>Deallocation:</span><span>count == 0</span></div>
                  </div>
                </div>
              </div>

              <h2 className="section-title"><Sparkles size={20} style={{ color: 'var(--color-orange-primary)' }} /> Critical Memory Innovations</h2>
              <div className="callout callout-warning">
                <div className="callout-icon"><RefreshCw size={20} style={{ color: 'var(--text-warning)' }} /></div>
                <div className="callout-content">
                  <div className="callout-title">1. Loop Bookmark Optimization (Flat RAM Usage)</div>
                  <div className="callout-text">
                    In Apollo loops, the compiler emits <code>apl_arena_get_mark()</code> at the start of an iteration and resets to that mark at the end. This guarantees memory consumption stays completely flat even across 10,000,000 loop iterations.
                  </div>
                </div>
              </div>

              <div className="callout callout-emerald">
                <div className="callout-icon"><ArrowRight size={20} style={{ color: 'var(--color-emerald-primary)' }} /></div>
                <div className="callout-content">
                  <div className="callout-title">2. Parent Arena Return Pattern (Zero-Cost Returns)</div>
                  <div className="callout-text">
                    When a child function returns dynamic data, the compiler passes a hidden <code>target_return_arena</code> pointer. The child allocates the return value directly in the parent's arena, avoiding ARC overhead and escape analysis.
                  </div>
                </div>
              </div>
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 8: INTERACTIVE MEMORY EXPLORER */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'memory-explorer' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-purple">Interactive Tool</span>
                <span className="badge badge-orange">Memory Classifier</span>
              </div>
              <h1 className="docs-title">Interactive Memory Bucket Classifier</h1>
              <div className="docs-description">Click on language constructs below to see how Apollo's compiler categorizes memory allocation in real time.</div>
            </div>

            <section>
              <div className="memory-explorer-grid">
                <div className="explorer-item-list">
                  {Object.keys(memoryCatalog).map(key => {
                    const item = memoryCatalog[key];
                    return (
                      <button
                        key={key}
                        className={`explorer-item-btn ${selectedMemoryItem === key ? 'selected' : ''}`}
                        onClick={() => setSelectedMemoryItem(key)}
                      >
                        <span style={{ fontWeight: 600, fontSize: '0.88rem' }}>{item.title}</span>
                        <span className={`badge ${item.badgeClass}`}>{item.bucket.split(':')[0]}</span>
                      </button>
                    );
                  })}
                </div>

                <div className="explorer-detail-card">
                  {(() => {
                    const item = memoryCatalog[selectedMemoryItem];
                    return (
                      <>
                        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                          <span className={`badge ${item.badgeClass}`}>{item.bucket}</span>
                          <span style={{ fontSize: '0.75rem', fontFamily: 'var(--font-mono)', color: 'var(--text-muted)' }}>ARC: {item.arcOverhead}</span>
                        </div>
                        <h3 style={{ margin: '0', fontSize: '1.25rem', color: 'var(--text-primary)' }}>{item.title}</h3>
                        <p style={{ margin: 0, fontSize: '0.9rem', color: 'var(--text-secondary)' }}>{item.desc}</p>
                        
                        <div style={{ background: 'rgba(0,0,0,0.3)', padding: '0.9rem', borderRadius: '8px', border: '1px solid var(--color-border)' }}>
                          <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: '0.4rem', fontFamily: 'var(--font-mono)' }}>Generated LLVM IR:</div>
                          <pre style={{ margin: 0, fontFamily: 'var(--font-mono)', fontSize: '0.82rem', color: '#38bdf8' }}>
                            <code>{item.ir}</code>
                          </pre>
                        </div>

                        <div className="bucket-meta">
                          <div className="bucket-meta-item"><span>Allocation Speed:</span><span>{item.allocLatency}</span></div>
                          <div className="bucket-meta-item"><span>Deallocation Speed:</span><span>{item.deallocLatency}</span></div>
                          <div className="bucket-meta-item"><span>ARC Reference Count:</span><span>{item.arcOverhead}</span></div>
                        </div>
                      </>
                    );
                  })()}
                </div>
              </div>
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 9: MODULES */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'modules' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-cyan">apollo-modules</span>
                <span className="badge badge-purple">LLVM Bitcode</span>
              </div>
              <h1 className="docs-title">Standard Runtime Modules</h1>
              <div className="docs-description">Pre-compiled C sub-libraries compiled to LLVM bitcode (.bc) and linked into native binaries.</div>
            </div>

            <section>
              <div className="docs-table-wrapper">
                <table className="docs-table">
                  <thead>
                    <tr>
                      <th>Module</th>
                      <th>Location</th>
                      <th>Primary Responsibilities</th>
                      <th>Bitcode File</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr>
                      <td><strong style={{ color: 'var(--color-orange-light)' }}>apl-io</strong></td>
                      <td><code>apollo-modules/apl-io/</code></td>
                      <td>Formatted console output (<code>_apl_print_int</code>, <code>_apl_print_float</code>, <code>_apl_print_str</code>, bounds panic)</td>
                      <td><code>apl-io.bc</code></td>
                    </tr>
                    <tr>
                      <td><strong style={{ color: 'var(--color-purple-primary)' }}>apl-mem</strong></td>
                      <td><code>apollo-modules/apl-mem/</code></td>
                      <td>3+1 Bucket memory engine (64KB Arena chunk allocator, loop bookmarks, Heap ARC manager)</td>
                      <td><code>apl-mem.bc</code></td>
                    </tr>
                    <tr>
                      <td><strong style={{ color: 'var(--color-cyan-primary)' }}>apl-string</strong></td>
                      <td><code>apollo-modules/apl-string/</code></td>
                      <td>String allocation, slice manipulation, concatenation, and length helpers</td>
                      <td><code>apl-string.bc</code></td>
                    </tr>
                    <tr>
                      <td><strong style={{ color: 'var(--color-emerald-primary)' }}>apl-sys</strong></td>
                      <td><code>apollo-modules/apl-sys/</code></td>
                      <td>System-level utilities, memory allocation wrappers, OS process operations</td>
                      <td><code>apl-sys.bc</code></td>
                    </tr>
                  </tbody>
                </table>
              </div>

              <h2 className="section-title"><RefreshCw size={20} /> Re-Compiling Runtime Modules</h2>
              <CodeBlock 
                title="Bitcode Compilation Command"
                language="bash"
                code={`cd apollo-modules
./compile.sh apl-io
./compile.sh apl-mem
./compile.sh apl-string
./compile.sh apl-sys`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 10: PIPELINE */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'pipeline' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange">Multi-Pass Compiler</span>
                <span className="badge badge-purple">libLLVM-19</span>
              </div>
              <h1 className="docs-title">Compiler Pipeline &amp; LLVM Codegen</h1>
              <div className="docs-description">How Apollo source code (.apl) transforms into an optimized native Windows executable.</div>
            </div>

            <section>
              <div className="pipeline-flow">
                <div className="pipeline-node active">
                  <div className="pipeline-node-title">Source</div>
                  <div className="pipeline-node-desc">.apl code</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Lexer</div>
                  <div className="pipeline-node-desc">gperf hash</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Parser</div>
                  <div className="pipeline-node-desc">Pratt AST</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">Semantic</div>
                  <div className="pipeline-node-desc">Type Check</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node">
                  <div className="pipeline-node-title">LLVM 19</div>
                  <div className="pipeline-node-desc">Bitcode (.bc)</div>
                </div>
                <div className="pipeline-arrow"><ChevronRight size={18} /></div>
                <div className="pipeline-node active">
                  <div className="pipeline-node-title">Clang</div>
                  <div className="pipeline-node-desc">program.exe</div>
                </div>
              </div>

              <h2 className="section-title"><Cpu size={20} /> Compilation Steps</h2>
              <ol style={{ paddingLeft: '1.5rem', color: 'var(--text-secondary)' }}>
                <li style={{ marginBottom: '0.8rem' }}><strong>Lexical Scanning (`src/lexer.c`):</strong> Converts characters to tokens using GNU gperf $O(1)$ keyword hash lookup.</li>
                <li style={{ marginBottom: '0.8rem' }}><strong>Recursive Descent Pratt Parser (`src/Parser/*`):</strong> Builds polymorphic AST nodes with precedence climbing and panic-mode error recovery.</li>
                <li style={{ marginBottom: '0.8rem' }}><strong>Semantic Analysis (`src/Semantics/*`):</strong> Validates types, symbol tables, pointer indirection levels, and scope boundaries.</li>
                <li style={{ marginBottom: '0.8rem' }}><strong>LLVM Code Generation (`src/API/*`):</strong> Emits LLVM IR bitcode using the official `libLLVM-19` C API and links runtime bitcodes.</li>
                <li style={{ marginBottom: '0.8rem' }}><strong>Clang Native Linking:</strong> Optimizes and links bitcode to native Windows PE executable (`.exe`).</li>
              </ol>
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 11: DIAGNOSTICS & TESTING */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'diagnostics' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-emerald">QA &amp; Tests</span>
                <span className="badge badge-purple">Panic-Mode Recovery</span>
              </div>
              <h1 className="docs-title">Error Recovery &amp; Testing Suite</h1>
              <div className="docs-description">Panic-Mode synchronization and automated Python testing harness.</div>
            </div>

            <section>
              <h2 className="section-title"><ShieldCheck size={20} /> Panic-Mode Error Recovery</h2>
              <p>
                When the parser encounters a syntax error, instead of crashing or generating cascading phantom errors, it invokes <code>synchronize()</code> to advance to the next statement delimiter (e.g. <code>;</code> or <code>&#125;</code>).
              </p>

              <h2 className="section-title"><Terminal size={20} style={{ color: 'var(--color-orange-primary)' }} /> Automated Python Test Runner</h2>
              <CodeBlock 
                title="Running Test Suite"
                language="powershell"
                code={`# Run complete automated test suite
python tests/test.py

# Run through diagnosis script
./diagnosis.sh println
./diagnosis.sh conditionals
./diagnosis.sh variable
./diagnosis.sh fxns`}
              />
            </section>
          </div>
        )}

        {/* ------------------------------------------------------------- */}
        {/* TAB 12: PLAYGROUND & LAB */}
        {/* ------------------------------------------------------------- */}
        {activeTab === 'playground' && (
          <div>
            <div className="docs-header">
              <div className="badge-row">
                <span className="badge badge-orange"><Terminal size={12} /> Interactive Lab</span>
                <span className="badge badge-purple">Real-Time Simulation</span>
              </div>
              <h1 className="docs-title">Interactive Compiler Visualizer</h1>
              <div className="docs-description">Write Apollo code and inspect tokens, AST hierarchy, generated LLVM IR, and simulated stdout execution!</div>
            </div>

            <section>
              <div className="playground-layout">
                {/* Editor Panel */}
                <div className="panel">
                  <div className="panel-header">
                    <div className="panel-title">
                      <Code size={16} style={{ color: 'var(--color-orange-light)' }} />
                      <span>Apollo Source Editor</span>
                    </div>
                    <div style={{ display: 'flex', gap: '0.6rem', alignItems: 'center' }}>
                      <select 
                        className="playground-select" 
                        value={codeSample} 
                        onChange={handleSampleChange}
                      >
                        <option value="structs">Structs &amp; Methods</option>
                        <option value="pointers">Pointers &amp; Deref</option>
                        <option value="memory">3+1 Memory Buckets</option>
                        <option value="arrays">Arrays &amp; Loops</option>
                        <option value="loops">Countdown Loop</option>
                      </select>
                      <button className="btn-compile" onClick={runVisualizer}>
                        <Play size={14} fill="currentColor" />
                        <span>Run</span>
                      </button>
                    </div>
                  </div>

                  <textarea 
                    className="editor-textarea"
                    value={playgroundCode}
                    onChange={(e) => setPlaygroundCode(e.target.value)}
                    spellCheck="false"
                  />
                </div>

                {/* Output & Pipeline Visualizer Panel */}
                <div className="panel">
                  <div className="vis-steps">
                    <button 
                      className={`vis-step-btn ${visStep === 'output' ? 'active' : ''}`}
                      onClick={() => setVisStep('output')}
                    >
                      Console Output
                    </button>
                    <button 
                      className={`vis-step-btn ${visStep === 'tokens' ? 'active' : ''}`}
                      onClick={() => setVisStep('tokens')}
                    >
                      Tokens
                    </button>
                    <button 
                      className={`vis-step-btn ${visStep === 'ast' ? 'active' : ''}`}
                      onClick={() => setVisStep('ast')}
                    >
                      AST Tree
                    </button>
                    <button 
                      className={`vis-step-btn ${visStep === 'llvm' ? 'active' : ''}`}
                      onClick={() => setVisStep('llvm')}
                    >
                      LLVM IR
                    </button>
                  </div>

                  <div className="vis-content">
                    {pipelineError && (
                      <div className="callout callout-warning" style={{ margin: 0 }}>
                        <div className="callout-icon"><AlertCircle size={18} style={{ color: 'var(--text-error)' }} /></div>
                        <div className="callout-content">
                          <div className="callout-title" style={{ color: 'var(--text-error)' }}>Compilation Diagnostic</div>
                          <div className="callout-text">{pipelineError}</div>
                        </div>
                      </div>
                    )}

                    {!pipelineError && !pipelineResults && (
                      <div className="pipeline-status">
                        <Play size={32} style={{ opacity: 0.3 }} />
                        <span>Click "Run" to compile and inspect pipeline artifacts</span>
                      </div>
                    )}

                    {!pipelineError && pipelineResults && (
                      <>
                        {visStep === 'output' && (
                          <div>
                            <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: '0.5rem' }}>
                              === Native Binary Execution Simulation ===
                            </div>
                            <div className="terminal-output">
                              {pipelineResults.termLog.map((line, i) => (
                                <div key={i} className="terminal-line">{line}</div>
                              ))}
                            </div>
                          </div>
                        )}

                        {visStep === 'tokens' && (
                          <div className="token-grid">
                            {pipelineResults.tokens.map((tok, i) => (
                              <div key={i} className="token-card">
                                <span className="token-type">{tok.type}</span>
                                <span className="token-value">{tok.value}</span>
                                <span className="token-line">L{tok.line}</span>
                              </div>
                            ))}
                          </div>
                        )}

                        {visStep === 'ast' && (
                          <div>
                            {renderASTNode(pipelineResults.ast)}
                          </div>
                        )}

                        {visStep === 'llvm' && (
                          <pre style={{ margin: 0, color: '#38bdf8' }}>
                            <code>{pipelineResults.llvm}</code>
                          </pre>
                        )}
                      </>
                    )}
                  </div>
                </div>
              </div>
            </section>
          </div>
        )}

      </main>
    </div>
  );
}
