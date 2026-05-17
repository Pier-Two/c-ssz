#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FORMAL_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if ! command -v coqc >/dev/null 2>&1 || ! command -v clightgen >/dev/null 2>&1; then
  if command -v opam >/dev/null 2>&1 && opam switch list --short | grep -qx "vst"; then
    eval "$(opam env --switch=vst --set-switch)"
  fi
fi

if ! command -v coqc >/dev/null 2>&1 || ! command -v clightgen >/dev/null 2>&1; then
  cat >&2 <<'EOF'
Coq/Rocq or CompCert clightgen was not found.

Reproducible setup commands:
  opam switch create vst ocaml-base-compiler.4.14.2
  opam switch set vst
  opam install coq-compcert coq-vst
  eval "$(opam env --switch=vst --set-switch)"
EOF
  exit 127
fi

cd "${FORMAL_DIR}"
make verify
