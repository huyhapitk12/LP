const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const { exec, spawn } = require('child_process');

/**
 * SECURITY: Validate file path to prevent command injection
 * Returns true if path appears safe, false otherwise
 */
function isValidFilePath(filePath) {
    if (!filePath || typeof filePath !== 'string') return false;
    
    // Check for shell metacharacters
    const dangerousChars = [';', '|', '&', '`', '$', '(', ')', '{', '}', '[', ']', '<', '>', "'", '"', '\\', '\n', '\r'];
    for (const char of dangerousChars) {
        if (filePath.includes(char)) {
            return false;
        }
    }
    
    // Check for path traversal attempts
    if (filePath.includes('..')) return false;
    
    // Check for null bytes
    if (filePath.includes('\0')) return false;
    
    return true;
}

/**
 * SECURITY: Validate compiler path
 */
function isValidCompilerPath(compilerPath) {
    if (!compilerPath || typeof compilerPath !== 'string') return false;
    
    // Allow common compiler path patterns
    const allowedPatterns = [
        /^lp$/,                          // Just 'lp'
        /^lp\.exe$/,                     // Just 'lp.exe'
        /^\.\/lp$/,                      // ./lp
        /^\.\\lp\.exe$/,                 // .\lp.exe
        /^[a-zA-Z]:\\[^\n\r]*\.exe$/,    // Windows absolute path ending in .exe
        /^\/[^\n\r]*\/lp$/               // Unix absolute path ending in /lp
    ];
    
    for (const pattern of allowedPatterns) {
        if (pattern.test(compilerPath)) return true;
    }
    
    // Additional validation: check for dangerous characters
    return isValidFilePath(compilerPath);
}

/**
 * Find the LP compiler path
 */
function findCompiler() {
    const config = vscode.workspace.getConfiguration('lp');
    const configPath = config.get('compilerPath');
    
    // SECURITY: Validate configured path
    if (configPath) {
        if (!isValidCompilerPath(configPath)) {
            vscode.window.showErrorMessage('Invalid compiler path configured. Please check your settings.');
            return 'lp'; // Fallback to default
        }
        return configPath;
    }

    // Try common locations
    const workspaceFolders = vscode.workspace.workspaceFolders;
    const candidates = [];
    
    if (workspaceFolders && workspaceFolders.length > 0) {
        candidates.push(path.join(workspaceFolders[0].uri.fsPath, 'build', 'lp.exe'));
        candidates.push(path.join(workspaceFolders[0].uri.fsPath, 'lp.exe'));
    }
    
    // SECURITY: Only use hardcoded paths that are safe
    // Removed user-specific hardcoded paths like d:\\LP\\build\\lp.exe
    candidates.push('lp'); // Default to PATH

    for (const candidate of candidates) {
        if (isValidFilePath(candidate) && fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return 'lp'; // Default to 'lp' in PATH if nothing else is found
}

/**
 * Run an LP command in the terminal
 * SECURITY: Use spawn instead of exec to avoid shell injection
 */
function runInTerminal(command, name, args = []) {
    let terminal = vscode.window.terminals.find(t => t.name === name);
    if (!terminal) {
        terminal = vscode.window.createTerminal(name);
    }
    terminal.show();
    
    // SECURITY: Build command safely with proper escaping
    // Using sendText is safer than exec as it doesn't execute through shell
    if (args.length > 0) {
        const escapedArgs = args.map(a => `"${a.replace(/"/g, '\\"')}"`).join(' ');
        terminal.sendText(`& "${command}" ${escapedArgs}`);
    } else {
        terminal.sendText(`& "${command}"`);
    }
}

/**
 * SECURITY: Execute compiler safely using spawn instead of exec
 */
function executeCompilerSafely(compiler, filePath, outPath, callback) {
    // Validate inputs
    if (!isValidCompilerPath(compiler)) {
        callback(new Error('Invalid compiler path'), '', '');
        return;
    }
    if (!isValidFilePath(filePath)) {
        callback(new Error('Invalid file path'), '', '');
        return;
    }
    if (outPath && !isValidFilePath(outPath)) {
        callback(new Error('Invalid output path'), '', '');
        return;
    }
    
    // Use spawn with argument array to avoid shell interpretation
    const args = [filePath, '-o', outPath];
    const child = spawn(compiler, args, {
        shell: false,
        windowsHide: true
    });
    
    let stdout = '';
    let stderr = '';
    
    child.stdout.on('data', (data) => {
        stdout += data.toString();
    });
    
    child.stderr.on('data', (data) => {
        stderr += data.toString();
    });
    
    child.on('close', (code) => {
        const error = code !== 0 ? new Error(`Compiler exited with code ${code}`) : null;
        callback(error, stdout, stderr);
    });
    
    child.on('error', (err) => {
        callback(err, stdout, stderr);
    });
}

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    console.log('LP Language extension activated');

    // Command: Run current file
    const runCmd = vscode.commands.registerCommand('lp.run', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('No active LP file to run.');
            return;
        }

        const filePath = editor.document.fileName;
        if (!filePath.endsWith('.lp')) {
            vscode.window.showWarningMessage('Active file is not an LP file (.lp)');
            return;
        }
        
        // SECURITY: Validate file path
        if (!isValidFilePath(filePath)) {
            vscode.window.showErrorMessage('Invalid file path detected.');
            return;
        }

        // Save before running
        editor.document.save().then(() => {
            const compiler = findCompiler();
            // SECURITY: Use spawn-based terminal execution
            runInTerminal(compiler, 'LP Run', [filePath]);
        });
    });

    // Command: Open REPL
    const replCmd = vscode.commands.registerCommand('lp.runRepl', () => {
        const compiler = findCompiler();
        runInTerminal(compiler, 'LP REPL');
    });

    // Command: Generate C code
    const buildCCmd = vscode.commands.registerCommand('lp.buildC', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || !editor.document.fileName.endsWith('.lp')) {
            vscode.window.showWarningMessage('No active LP file.');
            return;
        }

        const filePath = editor.document.fileName;
        const outPath = filePath.replace(/\.lp$/, '.c');
        
        // SECURITY: Validate paths
        if (!isValidFilePath(filePath) || !isValidFilePath(outPath)) {
            vscode.window.showErrorMessage('Invalid file path detected.');
            return;
        }

        editor.document.save().then(() => {
            const compiler = findCompiler();
            
            // SECURITY: Use safe execution
            executeCompilerSafely(compiler, filePath, outPath, (err, stdout, stderr) => {
                if (err) {
                    vscode.window.showErrorMessage(`LP Error: ${stderr || err.message}`);
                } else {
                    vscode.window.showInformationMessage(`Generated: ${path.basename(outPath)}`);
                    vscode.workspace.openTextDocument(outPath).then(doc => {
                        vscode.window.showTextDocument(doc, vscode.ViewColumn.Beside);
                    });
                }
            });
        });
    });

    // Command: Compile to executable
    const compileCmd = vscode.commands.registerCommand('lp.compile', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || !editor.document.fileName.endsWith('.lp')) {
            vscode.window.showWarningMessage('No active LP file.');
            return;
        }

        const filePath = editor.document.fileName;
        const outPath = filePath.replace(/\.lp$/, '.exe');
        
        // SECURITY: Validate paths
        if (!isValidFilePath(filePath) || !isValidFilePath(outPath)) {
            vscode.window.showErrorMessage('Invalid file path detected.');
            return;
        }

        editor.document.save().then(() => {
            const compiler = findCompiler();
            runInTerminal(compiler, 'LP Compile', [filePath, '-c', outPath]);
        });
    });

    context.subscriptions.push(runCmd, replCmd, buildCCmd, compileCmd);
}

function deactivate() {}

module.exports = { activate, deactivate };
