# <%= projectName %>

An [MPL](https://github.com/stissshak/First-Compiler) project, by <%= author %>.

## Build & run

This project is compiled with the MPL compiler (`comp`). Make sure it is on your
`PATH`, or pass its location to `make`:

```sh
make                    # build src/main.mpl -> src/main.exe
make run                # build and run
make clean              # remove generated .asm/.o/.exe

make MPC=/path/to/comp  # if `comp` is not on PATH
```

## Layout

```
<%= projectName %>/
├── Makefile
├── src/
│   └── main.mpl        # program entry point
└── examples/           # sample MPL programs (optional)
```
