using LumenBuilder.Model;
using LumenBuilder.Parsing.Lexer;
using LumenBuilder.Parsing.Parser;

namespace LumenBuilder
{

    namespace Parsing
    {

        /// <summary>
        /// Build file pure Parser, without I/O.
        /// </summary>
        public sealed class BuildFileParser
        {

            /// <summary>
            /// Parses a build file source into a ModuleDescriptor.
            /// </summary>
            public ModuleDescriptor Parse(string ModuleName, string ModuleDir, ReadOnlySpan<char> Source)
            {
                var Memory = new ReadOnlyMemory<char>(Source.ToArray());
                var Lexer = new BuildLexer(Memory);
                var Tokens = Lexer.Tokenize();
                var Parser = new BuildParser(Tokens);
                var Ast = Parser.Parse();

                return ConvertToDescriptor(ModuleName, ModuleDir, Ast);
            }

            /// <summary>
            /// Converts an AST ModuleNode into a ModuleDescriptor.
            /// </summary>
            private static ModuleDescriptor ConvertToDescriptor(string ModuleName, string ModuleDir, ModuleNode Ast)
            {
                ModuleType Type = ModuleType.StaticLibrary;
                var Sources = new List<string>();
                var PublicIncludes = new List<string>();
                var PrivateIncludes = new List<string>();
                var Defines = new List<string>();
                var Dependencies = new List<string>();
                var LinuxDeps = new List<string>();
                var WindowsDeps = new List<string>();
                var MacOSDeps = new List<string>();
                var LinuxLibs = new List<string>();
                var WindowsLibs = new List<string>();
                var MacOSLibs = new List<string>();

                for (int PropIndex = 0; PropIndex < Ast.Properties.Count; PropIndex++)
                {
                    var Prop = Ast.Properties[PropIndex];
                    string PropName = Prop.Name.ToLowerInvariant();

                    switch (PropName)
                    {
                        case "type":
                            Type = ParseModuleType(Prop.Value);
                            break;
                        case "sources":
                            ExtractStrings(Prop.Value, Sources);
                            break;
                        case "public_includes":
                            ExtractStrings(Prop.Value, PublicIncludes);
                            break;
                        case "private_includes":
                            ExtractStrings(Prop.Value, PrivateIncludes);
                            break;
                        case "defines":
                            ExtractStrings(Prop.Value, Defines);
                            break;
                        case "deps":
                            ExtractIdentifiers(Prop.Value, Dependencies);
                            break;
                        case "linux_deps":
                            ExtractIdentifiers(Prop.Value, LinuxDeps);
                            break;
                        case "windows_deps":
                            ExtractIdentifiers(Prop.Value, WindowsDeps);
                            break;
                        case "macos_deps":
                            ExtractIdentifiers(Prop.Value, MacOSDeps);
                            break;
                        case "linux_libs":
                            ExtractIdentifiers(Prop.Value, LinuxLibs);
                            break;
                        case "windows_libs":
                            ExtractIdentifiers(Prop.Value, WindowsLibs);
                            break;
                        case "macos_libs":
                            ExtractIdentifiers(Prop.Value, MacOSLibs);
                            break;
                    }
                }

                var PlatformDeps = new PlatformDependencies(LinuxDeps, WindowsDeps, MacOSDeps);
                var PlatformLibs = new PlatformLibraries(LinuxLibs, WindowsLibs, MacOSLibs);

                return new ModuleDescriptor(
                    ModuleName,
                    ModuleDir,
                    Type,
                    Sources,
                    PublicIncludes,
                    PrivateIncludes,
                    Defines,
                    Dependencies,
                    PlatformDeps,
                    PlatformLibs
                );
            }

            /// <summary>
            /// Parses a module type from an AST node.
            /// </summary>
            private static ModuleType ParseModuleType(AstNode Node)
            {
                string Value = Node switch
                {
                    IdentifierNode Id => Id.Value.ToLowerInvariant(),
                    StringNode Str => Str.Value.ToLowerInvariant(),
                    _ => "static_library"
                };

                return Value switch
                {
                    "executable" => ModuleType.Executable,
                    "shared_library" or "sharedlibrary" or "shared" => ModuleType.SharedLibrary,
                    _ => ModuleType.StaticLibrary
                };
            }

            /// <summary>
            /// Extracts string values from an AST node into a target list.
            /// </summary>
            private static void ExtractStrings(AstNode Node, List<string> Target)
            {
                if (Node is ArrayNode Array)
                {
                    for (int ElemIndex = 0; ElemIndex < Array.Elements.Count; ElemIndex++)
                    {
                        if (Array.Elements[ElemIndex] is StringNode Str)
                            Target.Add(Str.Value);
                    }
                }
                else if (Node is StringNode Single)
                {
                    Target.Add(Single.Value);
                }
            }

            /// <summary>
            /// Extracts identifier values from an AST node into a target list.
            /// </summary>
            private static void ExtractIdentifiers(AstNode Node, List<string> Target)
            {
                if (Node is ArrayNode Array)
                {
                    for (int ElemIndex = 0; ElemIndex < Array.Elements.Count; ElemIndex++)
                    {
                        var Element = Array.Elements[ElemIndex];

                        if (Element is IdentifierNode Id)
                        {
                            Target.Add(Id.Value);
                        }
                        else if (Element is StringNode Str)
                        {
                            Target.Add(Str.Value);
                        }
                    }
                }
                else if (Node is IdentifierNode SingleId)
                {
                    Target.Add(SingleId.Value);
                }
            }

        }

    }

}
