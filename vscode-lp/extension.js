const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');

/**
 * Find the LP compiler path
 */
function findCompiler() {
    const config = vscode.workspace.getConfiguration('lp');
    const configPath = config.get('compilerPath');
    if (configPath) return configPath;

    // Try common locations
    const workspaceFolders = vscode.workspace.workspaceFolders;
    const candidates = [];
    
    if (workspaceFolders && workspaceFolders.length > 0) {
        candidates.push(path.join(workspaceFolders[0].uri.fsPath, 'build', 'lp.exe'));
        candidates.push(path.join(workspaceFolders[0].uri.fsPath, 'lp.exe'));
    }
    
    candidates.push('d:\\LP\\build\\lp.exe');
    candidates.push('c:\\LP\\build\\lp.exe');

    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return 'lp'; // Default to 'lp' in PATH if nothing else is found
}

/**
 * Run an LP command in the terminal
 */
function runInTerminal(command, name) {
    let terminal = vscode.window.terminals.find(t => t.name === name);
    if (!terminal) {
        terminal = vscode.window.createTerminal(name);
    }
    terminal.show();
    terminal.sendText(command);
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

        // Save before running
        editor.document.save().then(() => {
            const compiler = findCompiler();
            runInTerminal(`& "${compiler}" "${filePath}"`, 'LP Run');
        });
    });

    // Command: Open REPL
    const replCmd = vscode.commands.registerCommand('lp.runRepl', () => {
        const compiler = findCompiler();
        runInTerminal(`& "${compiler}"`, 'LP REPL');
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

        editor.document.save().then(() => {
            const compiler = findCompiler();
            exec(`"${compiler}" "${filePath}" -o "${outPath}"`, (err, stdout, stderr) => {
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

        editor.document.save().then(() => {
            const compiler = findCompiler();
            runInTerminal(`& "${compiler}" "${filePath}" -c "${outPath}"`, 'LP Compile');
        });
    });

    context.subscriptions.push(runCmd, replCmd, buildCCmd, compileCmd);
}

function deactivate() {}

module.exports = { activate, deactivate };
