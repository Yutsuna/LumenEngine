@{
    RootModule        = 'LumenDeps.psm1'
    ModuleVersion     = '1.0.0'
    GUID              = 'b8e3c1a2-4f7d-4e9b-9c5a-3d2f1e0a8b6c'
    Author            = 'Lumen'
    CompanyName       = 'Lumen'
    Copyright         = '(c) 2026 Lumen. All rights reserved.'
    Description       = 'Windows dependency manager for Lumen using the MSYS2 UCRT64 toolchain.'

    PowerShellVersion = '5.1'

    FunctionsToExport = @('Invoke-LumenSetup')
    CmdletsToExport   = @()
    VariablesToExport = @()
    AliasesToExport   = @()

    RequiredModules   = @()
}
