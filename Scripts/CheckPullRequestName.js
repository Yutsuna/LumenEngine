/**
 * check pull request name
 * author: @leorevoir
 */

/**
 * constants
 */

const TYPES = [
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
];

const PATTERN = new RegExp(`^(${TYPES.join("|")})(\\(.+\\))?: .+`);

/**
 * helper
 */

function getErrorMessage(title) {
  const typesList = TYPES.map((type) => `- ${type}`).join("\n");

  return (
    `PR title "${title}" does not follow the Conventional Commits format.\n` +
    `Format required: <type>(<scope>): message\n` +
    `Where <type> is one of:\n${typesList}\n` +
    `Example: feat(auth): add login functionality`
  );
}

/**
 * entry-point
 */

module.exports = async ({ github, context, core }) => {
  const title = context.payload.pull_request.title;

  if (!PATTERN.test(title)) {
    core.setFailed(getErrorMessage(title));
  } else {
    console.log(`PR title "${title}" follows the Conventional Commits format.`);
  }
};
