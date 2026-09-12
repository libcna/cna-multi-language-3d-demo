# TypeScript port

`starfield.ts` is a typed, headless implementation of the shared simulation.
Compile it with a TypeScript toolchain and run the emitted file with Node, for
example `starfield.js win`. The desktop renderer remains an adapter boundary,
not a source of game rules.