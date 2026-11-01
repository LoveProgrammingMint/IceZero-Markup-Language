# IceZero Markup Language

IceZero Markup Language (IZML) is a strongly typed declarative data language and
its compilation toolchain. IZML documents describe templates (`define`, `enum`)
and data instances (`using`). The toolchain parses them into an intermediate
representation (IZLR), compiles the data to IZRC, and renders target-language
code through configuration files.

## Pipeline

```text
IZML source
  -> izml-parser        IZML -> IZLR
  -> izmlc              IZLR -> IZRC
  -> izmlgenc           IZLR -> target language
  -> runtime            IZRC -> language-native instances
```

The parser exports the C API in `parser/include/izml_parser_c_api.h` and the
resolved IZLR model in `common/include/izml_ir.hpp`. `izmlc` writes the compact
IZRC container. `izmlgenc` is language-neutral: it renders `.izmlgen-lang`
configuration files that carry the runtime wrapper, type and reader maps,
macros and output templates.

## Layout

```text
common/                 shared IZLR and I/O headers
parser/                 IZML lexer, parser, semantic analysis and C API
compiler/               izmlc and the IZRC writer
generator/              izmlgenc template engine and language-config loader
runtime/                native IZRC runtime (C ABI and C++ header)
data/izmlgen/           required .izmlgen-lang configuration files
licenses/               MINT license texts
```

## Build

```powershell
cmake --preset clang-release
cmake --build --preset clang-release
```

The Release preset enables `-O3`, `/Ob3`, `/OPT:REF`, `/OPT:ICF` and ThinLTO
(`-flto=thin`, consumed by `lld-link`). Disable them with
`-DIZML_AGGRESSIVE_OPTIMIZATION=OFF` or `-DIZML_ENABLE_THIN_LTO=OFF`.

Artifacts land in `build/clang-release/bin/`:

```text
izml-parser.dll
izml-runtime.dll
izmlc.exe
izmlgenc.exe
*.izmlgen-lang
```

`izmlgenc` discovers all `.izmlgen-lang` files in the current directory, falling
back to the directory beside the executable. Select a language by the `lang`
key in the file header, not by the file name:

```powershell
izmlgenc model.izml --lang c-cpp -o gen --bundle-runtime
izmlgenc model.izml --lang csharp -o gen --bundle-runtime
izmlgenc model.izml --lang python3 -o gen --bundle-runtime
```

`izmlc` compiles data without a subcommand:

```powershell
izmlc model.izml -o model.izrc
```

## License

The project uses the MINT License Suite.

- Source code (`common/`, `parser/`, `compiler/`, `generator/`, `runtime/`):
  `SPDX-License-Identifier: LicenseRef-MINT-1.0 AND LicenseRef-MINT-CODE-1.0`
- Data protocol files (`data/izmlgen/*.izmlgen-lang`):
  `SPDX-License-Identifier: LicenseRef-MINT-1.0 AND LicenseRef-MINT-DATA-1.0`

Component mapping is declared in `MINT-COMPONENTS.toml`. The base license and
both modules are included in `licenses/`.