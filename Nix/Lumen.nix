{ pkgs }:

let
  pythonEnv = pkgs.python3.withPackages (
    p: with p; [
      invoke
    ]
  );
  pythonScript = ''
    import sys
    import os
    import importlib.util
    from pathlib import Path
    workspace = Path(os.environ.get('WORKSPACE_ROOT', '.')).resolve()
    sys.path.insert(0, str(workspace / 'Scripts'))
    sys.argv[0] = 'lumen'
    try:
      entrypoint = workspace / 'Scripts' / 'LumenBuild' / 'Tasks.py'
      spec = importlib.util.spec_from_file_location('lumen_tasks_entrypoint', entrypoint)
      if spec is None or spec.loader is None:
        raise ImportError(f'Cannot load build entrypoint at {entrypoint}')
      module = importlib.util.module_from_spec(spec)
      spec.loader.exec_module(module)
      program = module.program
      program.run()
    except ImportError as e:
        print(f'\033[91m[ERR] LumenBuild initialization failed: {e}\033[0m')
        sys.exit(84)
    except Exception as e:
        print(f'\033[91m[ERR] Critical error: {e}\033[0m')
        sys.exit(1)
  '';
in
pkgs.writeShellScriptBin "lumen" ''
  WORKSPACE_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)

  export WORKSPACE_ROOT
  export IN_NIX_SHELL="1"

  exec ${pythonEnv}/bin/python3 - "$@" <<'PY'
${pythonScript}
PY
''
