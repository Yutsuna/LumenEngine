{ pkgs }:

let
  pythonEnv = pkgs.python3.withPackages (
    p: with p; [
      invoke
    ]
  );
in
pkgs.writeShellScriptBin "lumen" ''
  set -euo pipefail

  WORKSPACE_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)

  export WORKSPACE_ROOT
  export IN_NIX_SHELL="1"
  export PYTHONPATH="$WORKSPACE_ROOT/Scripts''${PYTHONPATH:+:$PYTHONPATH}"

  LUMEN_TASKS="$WORKSPACE_ROOT/Scripts/LumenBuild"

  exec ${pythonEnv}/bin/invoke \
    --search-root $LUMEN_TASKS \
    --collection  Registry \
    "$@"

''
