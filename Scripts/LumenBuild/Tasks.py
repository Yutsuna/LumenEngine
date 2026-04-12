from invoke import Collection
from invoke import Program as InvokeProgram

from LumenBuild.Tasks.Build import BuildDebug as FBuildDebug
from LumenBuild.Tasks.Build import BuildRelease as FBuildRelease
from LumenBuild.Tasks.Build import BuildHelp as FBuildHelp
from LumenBuild.Tasks.Clean import Clean as FClean
from LumenBuild.Tasks.Format import Format as FFormat
from LumenBuild.Tasks.Helper import PrintHelp as FHelp
from LumenBuild.Tasks.Test import Test as FTest

build_ns = Collection("Tasks/Build")
build_ns.add_task(FBuildDebug, name="debug")
build_ns.add_task(FBuildRelease, name="release", debug=False)
build_ns.add_task(FBuildHelp, name="clean", clean=True)

ns = Collection()
ns.add_collection(build_ns)
ns.add_task(FTest, name="Tasks/Test")
ns.add_task(FFormat, name="Tasks/Format")
ns.add_task(FClean, name="Tasks/Clean")
ns.add_task(FHelp, name="Tasks/Help")

program = InvokeProgram(namespace=ns, version="0.1.0", description="Lumen Build Tasks")
