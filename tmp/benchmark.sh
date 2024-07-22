#!/bin/sh
set -eu

result_md="${TMPDIR-/tmp}/prootie_benchmark_result.md"

hyperfine \
	--warmup 2 \
	--min-runs 20 \
	--max-runs 1000 \
	--export-markdown "${result_md}" \
	"proot-distro login alpine -- /bin/sh -lc pwd" \
	"./prootie.sh login alpine -- /bin/sh -lc pwd" \
	"./prootie login alpine -- /bin/sh -lc pwd"

echo "Result file: ${result_md}"
