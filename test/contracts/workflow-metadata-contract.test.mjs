import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const root = new URL("../../", import.meta.url);
const read = (path) => readFile(new URL(path, root), "utf8");

test("CI and package metadata match supported bindings and fork identity", async () => {
  const [ci, publish, regenerate, cargo, packageText, pyproject, config, readme] =
    await Promise.all([
      read(".github/workflows/ci.yml"),
      read(".github/workflows/publish.yml"),
      read(".github/workflows/regnerate.yml"),
      read("Cargo.toml"),
      read("package.json"),
      read("pyproject.toml"),
      read("tree-sitter.json"),
      read("README.md"),
    ]);

  const packageJson = JSON.parse(packageText);
  const treeSitter = JSON.parse(config);
  const failures = [];
  const check = (condition, message) => {
    if (!condition) failures.push(message);
  };

  const requiredPaths = [
    ".github/workflows/**",
    "bindings/**",
    "queries/**",
    "test/**",
    "grammar.js",
    "src/**",
    "binding.gyp",
    "Cargo.toml",
    "Cargo.lock",
    "go.mod",
    "go.sum",
    "Package.swift",
    "package.json",
    "package-lock.json",
    "pyproject.toml",
    "setup.py",
    "tree-sitter.json",
    "CMakeLists.txt",
    "Makefile",
  ];
  for (const path of requiredPaths) {
    const occurrences = ci.split(`- ${path}`).length - 1;
    check(occurrences === 2, `CI paths cover ${path} for push and pull requests`);
  }

  for (const binding of ["node", "python", "go", "rust", "swift"]) {
    check(
      new RegExp(`test-${binding}: (true|\\$\\{\\{)`).test(ci),
      `CI enables the ${binding} binding`,
    );
  }
  check(ci.includes("uses: actions/setup-node@"), "CI selects Node before generation");
  check(ci.includes("node-version: \"22\""), "CI tests Node 22");
  check(ci.includes("fetch-depth: 0"), "scanner checkout fetches comparison history");
  check(
    ci.includes("github.event.pull_request.base.sha"),
    "scanner compares pull requests to the PR base",
  );
  check(
    ci.includes("github.event.before"),
    "scanner compares pushes to the push before SHA",
  );
  check(!ci.includes("git diff --quiet HEAD^"), "scanner never compares only HEAD^");

  for (const [name, workflow] of [
    ["CI", ci],
    ["publish", publish],
    ["regenerate", regenerate],
  ]) {
    const activeUses = [...workflow.matchAll(/^\s*uses:\s*([^\s#]+).*$/gm)].map(
      ([, value]) => value,
    );
    check(activeUses.length > 0, `${name} workflow invokes actions`);
    for (const action of activeUses) {
      check(/@[0-9a-f]{40}$/.test(action), `${name} pins ${action} to a full SHA`);
    }
  }

  const repository = "https://github.com/anortham/tree-sitter-razor";
  check(packageJson.repository === repository, "npm metadata uses the fork URL");
  check(cargo.includes(`repository = "${repository}"`), "Cargo metadata uses the fork URL");
  check(pyproject.includes(`Homepage = "${repository}"`), "Python metadata uses the fork URL");
  check(
    treeSitter.metadata.links.repository === repository,
    "tree-sitter metadata uses the fork URL",
  );
  check(readme.includes("github.com/anortham/tree-sitter-razor"), "README uses the fork URL");

  const maintainer = { name: "Alan Northam", email: "anortham@gmail.com" };
  check(
    packageJson.author?.name === maintainer.name &&
      packageJson.author?.email === maintainer.email,
    "npm metadata uses the maintainer identity",
  );
  check(
    cargo.includes(`authors = ["${maintainer.name} <${maintainer.email}>"]`),
    "Cargo metadata uses the maintainer identity",
  );
  check(
    pyproject.includes(`authors = [{ name = "${maintainer.name}", email = "${maintainer.email}" }]`),
    "Python metadata uses the maintainer identity",
  );
  check(
    treeSitter.metadata.authors.some(
      (author) => author.name === maintainer.name && author.email === maintainer.email,
    ),
    "tree-sitter metadata uses the maintainer identity",
  );

  check(
    packageJson.engines?.node === ">=22 <23",
    "npm metadata limits source builds to verified Node 22",
  );
  check(
    treeSitter.grammars[0]["file-types"].includes("razor") &&
      treeSitter.grammars[0]["file-types"].includes("cshtml"),
    "tree-sitter metadata registers razor and cshtml",
  );
  check(
    readme.includes("view=aspnetcore-10.0") &&
      readme.includes("/aspnet/core/blazor/components/"),
    "README links the .NET 10 Razor syntax and component references",
  );
  check(
    readme.includes("/aspnet/core/release-notes/aspnetcore-11") &&
      readme.includes("preview"),
    "README labels the .NET 11 preview release notes",
  );

  const activePublish = publish
    .split("\n")
    .filter((line) => !line.trimStart().startsWith("#"))
    .join("\n");
  check(
    !/package-(npm|crates|pypi)\.yml/.test(activePublish),
    "registry publication stays disabled",
  );

  assert.deepEqual(failures, []);
});
