from __future__ import annotations

import re
import shutil
from pathlib import Path

from invoke import task

from LumenBuild.Constants import EAnsiColor as C
from LumenBuild.Utils import Log, LogError, LogOk, LogWarn


def _ToSnakeCase(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def _GenerateCppTemplates(
    name: str,
    project_dir: Path,
) -> dict[Path, str]:
    macro_name = f"LUMEN_{_ToSnakeCase(name)}_ASSET_PATH"

    return {
        project_dir / "CMakeLists.txt": f"""
LumenModule(
    NAME            {name}
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch Renderer Engine Compiler
)

###########################################################

target_compile_definitions(
    {name}
    PRIVATE
    {macro_name}="${{CMAKE_CURRENT_SOURCE_DIR}}/{name}/Assets"
)
""",
        project_dir / "Public" / "Application.hpp": f"""
/**
 * @file Application.hpp
 * @brief Declaration of the F{name}Application class for the {name} application
 */

#pragma once

#include "CoreTypes.hpp"

#include "GameApplication.hpp"

#include "Assets/AssetCompiler.hpp"
#include "World/World.hpp"

namespace LumenEngine
{{

class F{name}Application final : public IGameApplication
{{
public:

    Int32 Initialize ( Int32 Argc, const AnsiChar *Argv[] ) override;
    void Tick ( const Float64 InDeltaTime ) override;
    void Shutdown () override;

private:

    TUniquePtr<Engine::FWorld> World;
    TUniquePtr<Engine::FAssetCompiler> AssetCompiler;
}};

}} // namespace LumenEngine
""",
        project_dir / "Private" / "Application.cpp": f"""
/**
 * @file Application.cpp
 * @brief Implementation of the F{name}Application class
 */

#include "Application.hpp"

#include "ErrorCodes.hpp"
#include "MessageHandler.hpp"

#include "Generic/GenericApplication.hpp"

#include "Logging/Logger.hpp"

#ifndef {macro_name}
    #define {macro_name} ""
#endif

namespace
{{

LUMEN_LOG_DEFINE_CATEGORY( Log{name}, "{name}" );

}}

/**
 * Public
 */

LumenEngine::Int32 LumenEngine::F{name}Application::Initialize ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED *Argv[] )
{{
    if ( not GPlatformApplication.IsValid() )
    {{
        return EErrorCode::Failure;
    }};

    GPlatformApplication->SetMessageHandler( MakeShared<F{name}ApplicationMessageHandler>() );

    World = MakeUnique<Engine::FWorld>();

    AssetCompiler = MakeUnique<Engine::FAssetCompiler>();
    AssetCompiler->Initialize( {macro_name} );

    LUMEN_LOG_INFO( Log{name}, "World initialized with Camera, Scene and Mesh actors." );
    return EErrorCode::Success;
}}

void LumenEngine::F{name}Application::Shutdown ()
{{
    AssetCompiler.Reset();
    World.Reset();
}}

void LumenEngine::F{name}Application::Tick ( const Float64 InDeltaTime )
{{
    AssetCompiler->Tick();
    World->Tick( InDeltaTime );
}}

/**
 * Private
 */

LUMEN_REGISTER_GAME_APPLICATION( LumenEngine::F{name}Application );
""",
        project_dir / "Public" / "MessageHandler.hpp": f"""
/**
 * @file MessageHandler.hpp
 * @brief Declaration of the F{name}ApplicationMessageHandler class
 */

#pragma once

#include "Generic/GenericApplicationMessageHandler.hpp"

namespace LumenEngine
{{

class F{name}ApplicationMessageHandler final : public FGenericApplicationMessageHandler
{{
public:

    void OnRequestExit () override;
}};

}} // namespace LumenEngine
""",
        project_dir / "Private" / "MessageHandler.cpp": f"""
/**
 * @file MessageHandler.cpp
 * @brief Message handler implementation for the {name} application
 */

#include "MessageHandler.hpp"
#include "LaunchEngineLoop.hpp"

void LumenEngine::F{name}ApplicationMessageHandler::OnRequestExit ()
{{
    GEngineLoop.RequestExit( "Event: Application exit requested" );
}}
""",
    }


def _GeneratePublicPrivateTemplates(name: str, project_dir: Path) -> None:
    cpp_templates: dict[Path, str] = _GenerateCppTemplates(name, project_dir)

    for path, content in cpp_templates.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")


def _GenerateAssetTemplates(project_dir: Path) -> None:
    ASSET_TYPES = ("Materials", "Meshes", "Shaders")

    for sub_dir in ASSET_TYPES:
        (project_dir / "Assets" / sub_dir).mkdir(parents=True, exist_ok=True)


def _GenerateTemplates(name: str, project_dir: Path) -> None:
    try:
        _GeneratePublicPrivateTemplates(name, project_dir)
        _GenerateAssetTemplates(project_dir)
        LogOk(f"Project '{name}' successfully initialized at {project_dir.resolve()}")

    except OSError as err:
        LogError(f"Failed to create project structure: {err}")


@task(
    name="initialize",
    help={
        "name": "Mandatory name of the project to create",
        "directory": "Parent directory where the project directory will be created",
        "force": "Force initialization even if the target directory already exists",
    },
)
def Initialize(ctx, name: str, directory: str = ".", force: bool = False) -> None:
    Log(
        f"{C.BOLD}Initializing project '{name}'{C.RESET}  —  LumenEngine",
        prefix="INIT ",
        color=C.CYAN,
    )

    if not re.match(r"^[a-zA-Z][a-zA-Z0-9_]*$", name):
        LogError(
            f"Invalid project name '{name}'. It must start with a letter and "
            "only contain alphanumeric characters or underscores."
        )
        return

    project_dir = Path(directory) / name

    if project_dir.exists():
        LogWarn(f"Target directory already exists: {project_dir.resolve()}")

        if not force:
            Log("Use the '--force' flag to overwrite the existing directory.")
            return

        else:
            Log("Overwriting existing directory due to '--force' flag.")
            shutil.rmtree(project_dir)

    _GenerateTemplates(name, project_dir)
