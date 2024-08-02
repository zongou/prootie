#!/bin/sh
set -eu

result_md="${TMPDIR-/tmp}/prootie_benchmark_result.md"

hyperfine \
	--warmup 2 \
	--min-runs 20 \
	--max-runs 100 \
	--export-markdown "${result_md}" \
	"proot-distro login alpine -- pwd" \
	"./prootie.sh login distros/alpine -- pwd" \
	"./prootie login distros/alpine -- pwd"

echo "Result file: ${result_md}"
