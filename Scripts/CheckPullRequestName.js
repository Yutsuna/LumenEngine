/**
 * @file CheckPullRequestName.js
 * @brief Validates pull request titles against Conventional Commits format
 */

/**
 * @constant Config
 * @brief Configuration for PR title validation
 * @detail Contains allowed types and regex pattern
 */
const Config = {
  Types: [
    "Feat",
    "Fix",
    "Chore",
    "Docs",
    "Style",
    "Refactor",
    "Perf",
    "Test",
    "CI",
    "Build",
  ],
  get Pattern() {
    return new RegExp(`^(${this.Types.join("|")})(\\(.+\\))?: .+`);
  },
};

/**
 * @class PullRequestValidator
 * @brief Validates pull request titles
 * @detail Provides methods to validate PR titles and generate error messages
 */
class PullRequestValidator {
  /**
   * @function GetErrorMessage
   * @brief Generates a detailed error message for invalid PR titles
   * @param Title - The PR title to validate
   * @returns Formatted error message
   */
  static GetErrorMessage(Title) {
    const TypeList = Config.Types.map((Type) => `  • ${Type}`).join("\n");

    return [
      `PR title does not follow Conventional Commits format`,
      ``,
      `Current title: "${Title}"`,
      ``,
      `Required format: <type>(<scope>): <message>`,
      ``,
      `Available types:`,
      TypeList,
      ``,
      `Examples:`,
      `  • Feat(ApplicationCore): Add MacOS window support`,
      `  • Fix(Renderer): Resolve shadow rendering glitch`,
      `  • Docs(Examples): Update BaseExample for new API changes`,
    ].join("\n");
  }

  /**
   * @function Validate
   * @brief Validates PR title format
   * @param Title - The PR title to validate
   * @returns true if valid, false otherwise
   */
  static Validate(Title) {
    return Config.Pattern.test(Title);
  }

  /**
   * @function LogSuccess
   * @brief Logs success message
   * @param Title - The validated PR title
   */
  static LogSuccess(Title) {
    console.log(`PR title follows Conventional Commits format: "${Title}"`);
  }
}

/**
 * @function EntryPoint
 * @param Github - GitHub API client
 * @param Context - GitHub Actions context
 * @param Core - GitHub Actions core utilities
 */
module.exports = async ({ github, context, core }) => {
  try {
    const Title = context.payload.pull_request?.title;

    if (!Title) {
      throw new Error("Pull request title not found in context");
    }

    if (!PullRequestValidator.Validate(Title)) {
      core.setFailed(PullRequestValidator.GetErrorMessage(Title));
      return;
    }

    PullRequestValidator.LogSuccess(Title);
  } catch (CatchedError) {
    core.setFailed(`Script execution failed: ${CatchedError.message}`);
    throw CatchedError;
  }
};
