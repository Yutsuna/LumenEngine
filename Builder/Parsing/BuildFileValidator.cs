/**
 * @file BuildFileValidator.cs
 * @brief Pure semantic validator for build file descriptors.
 */

using LumenBuilder.Model;

namespace LumenBuilder.Parsing
{

    /// <summary>
    /// Validation error result.
    /// </summary>
    public sealed record ValidationError(
        string ModuleName,
        string Message
    );

    /// <summary>
    /// Validation result containing errors if any.
    /// </summary>
    public sealed record ValidationResult(
        bool IsValid,
        IReadOnlyList<ValidationError> Errors
    )
    {
        public static ValidationResult Success()
        {
            return new ValidationResult(true, Array.Empty<ValidationError>());
        }

        public static ValidationResult Failure(IReadOnlyList<ValidationError> Errors)
        {
            return new ValidationResult(false, Errors);
        }
    }

    /// <summary>
    /// Pure semantic validator for module descriptors.
    /// Validates build file structure and constraints without side effects.
    /// </summary>
    public static class BuildFileValidator
    {

        /// <summary>
        /// Validates a single module descriptor.
        /// </summary>
        public static ValidationResult Validate(ModuleDescriptor Module)
        {
            var Errors = new List<ValidationError>();

            /** Validate module name */
            if (string.IsNullOrWhiteSpace(Module.Name))
            {
                Errors.Add(new ValidationError(Module.Name, "Module name cannot be empty"));
            }

            /** Validate module directory */
            if (string.IsNullOrWhiteSpace(Module.Directory))
            {
                Errors.Add(new ValidationError(Module.Name, "Module directory cannot be empty"));
            }

            /** Validate module type */
            if (!IsValidModuleType(Module.Type))
            {
                Errors.Add(new ValidationError(Module.Name, $"Invalid module type: {Module.Type}"));
            }

            /** Validate sources (allow empty for header-only modules) */
            if (Module.Sources.Count == 0 && Module.PublicIncludes.Count == 0)
            {
                Errors.Add(new ValidationError(Module.Name, "Module must have at least one source file pattern or public include"));
            }

            for (int SourceIndex = 0; SourceIndex < Module.Sources.Count; SourceIndex++)
            {
                string Source = Module.Sources[SourceIndex];
                if (string.IsNullOrWhiteSpace(Source))
                {
                    Errors.Add(new ValidationError(Module.Name, "Source pattern cannot be empty"));
                }
            }

            /** Validate include paths */
            for (int IncludeIndex = 0; IncludeIndex < Module.PublicIncludes.Count; IncludeIndex++)
            {
                string Include = Module.PublicIncludes[IncludeIndex];
                if (string.IsNullOrWhiteSpace(Include))
                {
                    Errors.Add(new ValidationError(Module.Name, "Public include path cannot be empty"));
                }
            }

            for (int IncludeIndex = 0; IncludeIndex < Module.PrivateIncludes.Count; IncludeIndex++)
            {
                string Include = Module.PrivateIncludes[IncludeIndex];
                if (string.IsNullOrWhiteSpace(Include))
                {
                    Errors.Add(new ValidationError(Module.Name, "Private include path cannot be empty"));
                }
            }

            /** Validate dependencies */
            for (int DepIndex = 0; DepIndex < Module.Dependencies.Count; DepIndex++)
            {
                string Dep = Module.Dependencies[DepIndex];
                if (string.IsNullOrWhiteSpace(Dep))
                {
                    Errors.Add(new ValidationError(Module.Name, "Dependency name cannot be empty"));
                }
            }

            /** Validate platform dependencies */
            ValidatePlatformDependencies(Module.Name, "Linux", Module.PlatformDeps.Linux, Errors);
            ValidatePlatformDependencies(Module.Name, "Windows", Module.PlatformDeps.Windows, Errors);
            ValidatePlatformDependencies(Module.Name, "MacOS", Module.PlatformDeps.MacOS, Errors);

            return Errors.Count > 0
                ? ValidationResult.Failure(Errors)
                : ValidationResult.Success();
        }

        /// <summary>
        /// Validates a collection of module descriptors.
        /// </summary>
        public static ValidationResult ValidateAll(IEnumerable<ModuleDescriptor> Modules)
        {
            var AllErrors = new List<ValidationError>();

            foreach (var Module in Modules)
            {
                var Result = Validate(Module);
                if (!Result.IsValid)
                {
                    foreach (var Error in Result.Errors)
                    {
                        AllErrors.Add(Error);
                    }
                }
            }

            return AllErrors.Count > 0
                ? ValidationResult.Failure(AllErrors)
                : ValidationResult.Success();
        }

        /// <summary>
        /// Validates platform-specific dependencies.
        /// </summary>
        private static void ValidatePlatformDependencies(
            string ModuleName,
            string PlatformName,
            IReadOnlyList<string> Dependencies,
            List<ValidationError> Errors)
        {
            for (int DepIndex = 0; DepIndex < Dependencies.Count; DepIndex++)
            {
                string Dep = Dependencies[DepIndex];
                if (string.IsNullOrWhiteSpace(Dep))
                {
                    Errors.Add(new ValidationError(ModuleName, $"{PlatformName} dependency name cannot be empty"));
                }
            }
        }

        /// <summary>
        /// Checks if module type is valid.
        /// </summary>
        private static bool IsValidModuleType(ModuleType Type)
        {
            return Type == ModuleType.Executable
                || Type == ModuleType.SharedLibrary
                || Type == ModuleType.StaticLibrary;
        }

    }

}
