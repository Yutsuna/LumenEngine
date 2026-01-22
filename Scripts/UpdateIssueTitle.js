/**
 * updates github issue title based on its body and labels
 * author: @leorevoir
 */

/**
 * constants
 */

const PREFIX_MAP = {
  bug: "Fix",
  feature: "Feat",
  documentation: "Docs",
  "ci/cd": "CI",
  test: "Test",
  perf: "Perf",
  refactor: "Refactor",
  style: "Style",
  chore: "Chore",
};

async function getIssue({ github, context, isDispatch }) {
  if (isDispatch) {
    const issueNumber = parseInt(context.payload.inputs.issue_number);
    console.log(
      "Dispatch input issue_number:",
      context.payload.inputs.issue_number,
    );
    console.log("Parsed issueNumber:", issueNumber);

    const { data } = await github.rest.issues.get({
      owner: context.repo.owner,
      repo: context.repo.repo,
      issue_number: issueNumber,
    });
    return data;
  }
  return context.payload.issue;
}

function generateTitle(body, labels) {
  const prefix = labels.find((l) => PREFIX_MAP[l])
    ? PREFIX_MAP[labels.find((l) => PREFIX_MAP[l])]
    : "Chore";

  const extract = (regex) => {
    const m = body.match(regex);
    return m ? m[1].trim() : "";
  };

  const contextValue = extract(/### Context\s*([\s\S]*?)(?=###|$)/);
  const summaryValue = extract(/### Summary\s*([\s\S]*?)(?=###|$)/);

  if (!contextValue || !summaryValue) {
    return null;
  }

  return `${prefix}(${contextValue}): ${summaryValue}`;
}

function logDryRun(issueNumber, currentTitle, newTitle) {
  console.log(`[DRY_RUN] issue #${issueNumber}`);
  console.log(`[DRY_RUN] current title: ${currentTitle}`);
  console.log(`[DRY_RUN] new title: ${newTitle}`);
}

async function updateIssueTitle({ github, context, issueNumber, newTitle }) {
  await github.rest.issues.update({
    owner: context.repo.owner,
    repo: context.repo.repo,
    issue_number: issueNumber,
    title: newTitle,
  });
  console.log(`Updated issue #${issueNumber} title to: ${newTitle}`);
}

async function addContextLabelIfMissing({
  github,
  context,
  issueNumber,
  labels,
  body,
}) {
  const contextLabel = (body.match(/### Context\s*([\s\S]*?)(?=###|$)/) ||
    [])[1]?.trim();
  if (!contextLabel || labels.includes(contextLabel)) return;

  await github.rest.issues.addLabels({
    owner: context.repo.owner,
    repo: context.repo.repo,
    issue_number: issueNumber,
    labels: [contextLabel],
  });
  console.log(`Added label: ${contextLabel}`);
}

/**
 * entry-point
 */

module.exports = async ({ github, context }) => {
  console.log("________________________________");
  console.log("Event name:", context.eventName);

  const isDispatch = context.eventName === "workflow_dispatch";
  console.log("Is workflow dispatch:", isDispatch);

  const issue = await getIssue({ github, context, isDispatch });
  const issueNumber = issue.number;

  console.log("Final issueNumber:", issueNumber);
  console.log("Issue title:", issue.title);
  console.log("________________________________");

  if (!issueNumber) {
    throw new Error("Invalid issue number");
  }

  const dryRun = isDispatch && context.payload.inputs.dry_run === "true";
  const { title: currentTitle, body = "", labels = [] } = issue;
  const labelNames = labels.map((l) => (typeof l === "string" ? l : l.name));

  const newTitle = generateTitle(body, labelNames);

  if (!newTitle) {
    console.log("Missing required context or summary in issue body");
    return;
  }

  if (newTitle === currentTitle) {
    console.log("Title already up to date");
    return;
  }

  if (dryRun) {
    logDryRun(issueNumber, currentTitle, newTitle);
    return;
  }

  await updateIssueTitle({ github, context, issueNumber, newTitle });
  await addContextLabelIfMissing({
    github,
    context,
    issueNumber,
    labels: labelNames,
    body,
  });
};
