from invoke import (
    Collection as InvokeCollection,
)
from invoke import (
    Program as InvokeProgram,
)

from LumenBuild.Tasks.Build import (
    BuildDebug as FBuildDebug,
)
from LumenBuild.Tasks.Build import (
    BuildHelp as FBuildHelp,
)
from LumenBuild.Tasks.Build import (
    BuildRelease as FBuildRelease,
)
from LumenBuild.Tasks.Clean import Clean as FClean
from LumenBuild.Tasks.Format import Format as FFormat
from LumenBuild.Tasks.Helper import PrintHelp as FHelp
from LumenBuild.Tasks.Initialize import Initialize as FInitialize
from LumenBuild.Tasks.Test import Test as FTest
from LumenBuild.Tasks.Tidy import Tidy as FTidy

build_ns = InvokeCollection("build")
build_ns.add_task(FBuildDebug, name="debug")
build_ns.add_task(FBuildRelease, name="release")
build_ns.add_task(FBuildHelp, name="help")

ns = InvokeCollection()
ns.add_collection(build_ns)
ns.add_task(FTest, name="test")
ns.add_task(FFormat, name="format")
ns.add_task(FTidy, name="tidy")
ns.add_task(FClean, name="clean")
ns.add_task(FHelp, name="help")
ns.add_task(FInitialize, name="init")

program = InvokeProgram(namespace=ns, version="0.1.0")
