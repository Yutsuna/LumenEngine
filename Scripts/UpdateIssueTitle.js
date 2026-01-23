/**
 * @file UpdateIssueTitle.js
 * @brief Updates GitHub issue titles based on body content and labels
 */

/**
 * @constant Config
 * @brief Configuration for issue title generation
 * @detail Contains prefix mappings and regex patterns
 */
const Config = {
  PrefixMap: {
    bug: "Fix",
    enhancement: "Feat",
    feature: "Feat",
    documentation: "Docs",
    "ci/cd": "CI",
    test: "Test",
    perf: "Perf",
    refactor: "Refactor",
    style: "Style",
    chore: "Chore",
  },
  DefaultPrefix: "Chore",
  Patterns: {
    Context: /### Context\s*([\s\S]*?)(?=###|$)/,
    Summary: /### Summary\s*([\s\S]*?)(?=###|$)/,
  },
};

/**
 * @class IssueData
 * @brief Represents GitHub issue data
 * @detail Parses issue properties for easier access
 */
class IssueData {
  constructor(Issue) {
    this.Number = Issue.number;
    this.Title = Issue.title;
    this.Body = Issue.body || "";
    this.Labels = this.ParseLabels(Issue.labels || []);
  }

  /**
   * @function ParseLabels
   * @brief Parses issue labels into an array of strings
   * @return Array of label names
   */
  ParseLabels(Labels) {
    return Labels.map((Label) =>
      typeof Label === "string" ? Label : Label.name,
    );
  }
}

/**
 * @class IssueService
 * @brief Service for interacting with GitHub issues
 * @detail Provides methods to get and update issues
 */
class IssueService {
  constructor(Github, Context) {
    this.Github = Github;
    this.Context = Context;
    this.Owner = Context.repo.owner;
    this.Repo = Context.repo.repo;
  }

  /**
   * @function GetIssue
   * @brief Retrieves issue data based on event type
   * @param isDispatch - Whether this is a workflow dispatch event
   * @return Issue data
   */
  async GetIssue(bIsDispatch) {
    if (bIsDispatch) {
      return await this.GetIssueFromDispatch();
    }
    return this.Context.payload.issue;
  }

  /**
   * @function GetIssueFromDispatch
   * @brief Retrieves issue from workflow dispatch input
   * @return Issue data
   */
  async GetIssueFromDispatch() {
    const IssueNumber = parseInt(this.Context.payload.inputs.issue_number, 10);

    if (isNaN(IssueNumber) || IssueNumber <= 0) {
      throw new Error(
        `Invalid issue number: ${this.Context.payload.inputs.issue_number}`,
      );
    }

    console.log(`Fetching issue #${IssueNumber} from workflow dispatch`);

    const { Data } = await this.Github.rest.issues.get({
      owner: this.Owner,
      repo: this.Repo,
      issue_number: IssueNumber,
    });

    return Data;
  }

  /**
   * @function UpdateTitle
   * @brief Updates issue title
   * @param IssueNumber - Issue number
   * @param NewTitle - New title
   */
  async UpdateTitle(IssueNumber, NewTitle) {
    await this.Github.rest.issues.update({
      owner: this.Owner,
      repo: this.Repo,
      issue_number: IssueNumber,
      title: NewTitle,
    });
    console.log(`Updated issue #${IssueNumber} title to: "${NewTitle}"`);
  }

  /**
   * @function AddLabel
   * @brief Adds a label to an issue if not already present
   * @param IssueNumber - Issue number
   * @param Label - Label to add
   */
  async AddLabel(IssueNumber, Label) {
    await this.Github.rest.issues.addLabels({
      owner: this.Owner,
      repo: this.Repo,
      issue_number: IssueNumber,
      labels: [Label],
    });
    console.log(`Added label: "${Label}"`);
  }
}

/**
 * @class TitleGenerator
 * @brief Generates issue titles based on body content and labels
 * @detail Uses regex patterns to extract fields and constructs titles
 */
class TitleGenerator {
  /**
   * @function ExtractField
   * @brief Extracts field value from issue body using regex
   * @param Body - Issue body
   * @param Pattern - Regex pattern
   * @return Extracted value or empty string
   */
  static ExtractField(Body, Oattern) {
    const match = Body.match(Pattern);
    return match?.[1]?.trim() || "";
  }

  /**
   * @function GetPrefix
   * @brief Determines the prefix based on issue labels
   * @param Labels - Issue labels
   * @return Prefix type
   * @private
   */
  static GetPrefix(Labels) {
    const matchedLabel = Labels.find((Label) => Config.PrefixMap[Label]);
    return matchedLabel ? Config.PrefixMap[matchedLabel] : Config.DefaultPrefix;
  }

  /**
   * @function Generate
   * @brief Generates new issue title from body and labels
   * @param Body - Issue body
   * @param Labels - Issue labels
   * @return Generated title or null if invalid
   */
  static Generate(Body, Labels) {
    const Context = this.ExtractField(Body, Config.Patterns.Context);
    const Summary = this.ExtractField(Body, Config.Patterns.Summary);

    if (!Context || !Summary) {
      console.log("Missing required fields (Context or Summary)");
      return null;
    }

    const Prefix = this.GetPrefix(Labels);
    return `${Prefix}(${Context}): ${Summary}`;
  }
}

/**
 * @class IssueProcessor
 * @brief Processes issue title updates
 * @detail Handles title generation, dry run logging, and label addition
 */
class IssueProcessor {
  constructor(IssueService) {
    this.IssueService = IssueService;
  }

  /**
   * @function LogDryRun
   * @brief Logs dry run information
   * @param Issue - Issue data
   * @param NewTitle - New title
   * @private
   */
  LogDryRun(Issue, NewTitle) {
    console.log("\nDRY RUN MODE");
    console.log("─".repeat(50));
    console.log(`Issue Number: #${Issue.Number}`);
    console.log(`Current Title: "${Issue.Title}"`);
    console.log(`New Title: "${NewTitle}"`);
    console.log("─".repeat(50));
  }

  /**
   * @function AddContextLabel
   * @brief Adds context label if missing
   * @param Issue - Issue data
   */
  async AddContextLabel(Issue) {
    const ContextLabel = TitleGenerator.ExtractField(
      Issue.Body,
      Config.Patterns.Context,
    );

    if (!ContextLabel || Issue.Labels.includes(ContextLabel)) {
      return;
    }

    await this.IssueService.AddLabel(Issue.Number, ContextLabel);
  }

  /**
   * @function Process
   * @brief Processes issue title update
   * @param Issue - Issue data
   * @param bDryRun - Whether to run in dry mode
   */
  async Process(Issue, bDryRun = false) {
    const NewTitle = TitleGenerator.Generate(Issue.Body, Issue.Labels);

    if (!NewTitle) {
      return;
    }

    if (NewTitle === Issue.Title) {
      console.log("Title already up to date");
      return;
    }

    if (bDryRun) {
      this.LogDryRun(Issue, NewTitle);
      return;
    }

    await this.IssueService.UpdateTitle(Issue.Number, NewTitle);
    await this.AddContextLabel(Issue);
  }
}

/**
 * @function EntryPoint
 * @param Github - GitHub API client
 * @param Context - GitHub Actions context
 * @param Core - GitHub Actions core utilities
 */
export default module.exports = async ({ Github, Context, Core }) => {
  console.log("\n" + "=".repeat(50));
  console.log("Issue Title Update Workflow");
  console.log("=".repeat(50));
  console.log(`Event: ${Context.eventName}`);

  try {
    const bIsDispatch = Context.eventName === "workflow_dispatch";
    const bDryRun = bIsDispatch && Context.payload.inputs.dry_run === "true";

    const IssueService = new IssueService(Github, Context);
    const RawIssue = await IssueService.GetIssue(bIsDispatch);
    const Issue = new IssueData(RawIssue);

    console.log(`Issue: #${Issue.Number} - "${Issue.Title}"`);
    console.log("=".repeat(50) + "\n");

    const Processor = new IssueProcessor(IssueService);
    await Processor.Process(Issue, bDryRun);
  } catch (CatchedError) {
    console.error(`Error: ${CatchedError.message}`);
    Core.setFailed(`Script execution failed: ${CatchedError.message}`);
    throw CatchedError;
  }
};
