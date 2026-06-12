# MPL Language — VS Code extension

Syntax highlighting for **MPL**, the small C-like language compiled by
[First-Compiler](https://github.com/stissshak/First-Compiler). Activates on
`.mpl` files.

## Highlights

- Keywords (`if`, `for`, `return`, `struct`, `typedef`, `const`, `extern`, …)
- Built-in types (`int`, `uint`, `float`, `char`, `void`, `bool`, `byte`, `short`, `long`)
- Built-in functions (`print`, `input`, `malloc`, `free`, `assert`, `exit`, `panic`)
- Constants (`true`, `false`, `null`)
- Strings with escapes and `printf`-style placeholders (`%d`, `%s`, …)
- Numbers: decimal, float, `0x` hex, `0b` binary
- Line `//` and block `/* */` comments
- `#include` / `#define` preprocessor directives
- Function-call names, operators, bracket matching, auto-closing

## Install (local / development)

Copy or symlink this folder into your VS Code extensions directory, then
reload:

```sh
ln -s "$PWD/mpl-vscode" ~/.vscode/extensions/mpl-0.1.0
# then: Developer: Reload Window  (Ctrl+Shift+P)
```

## Install as a .vsix

```sh
npm install -g @vscode/vsce
cd mpl-vscode
vsce package            # produces mpl-0.1.0.vsix
code --install-extension mpl-0.1.0.vsix
```

## Files

| File | Purpose |
|------|---------|
| `package.json` | Extension manifest (`contributes.languages` + `grammars`) |
| `language-configuration.json` | Comments, brackets, auto-close, indentation |
| `syntaxes/mpl.tmLanguage.json` | TextMate grammar (the tokenizer) |
