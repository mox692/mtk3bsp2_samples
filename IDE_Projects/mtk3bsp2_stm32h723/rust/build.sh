#!/bin/bash
# Rust static library build script

cd "$(dirname "$0")"
cargo build --release

# Copy library to a convenient location for linking
cp target/thumbv7em-none-eabihf/release/libtron_rust.a ../libtron_rust.a
echo "Built: ../libtron_rust.a"
