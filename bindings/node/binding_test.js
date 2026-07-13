/// <reference types="node" />

const assert = require("node:assert/strict");
const { test } = require("node:test");

test("parses mixed Razor without errors", () => {
  const parser = new (require("tree-sitter"))();
  parser.setLanguage(require("."));

  const tree = parser.parse(
    '@page\n<EditForm Model="@model"><InputDate @bind-Value="model.Date" /></EditForm>',
  );

  assert.equal(tree.rootNode.type, "compilation_unit");
  assert.equal(tree.rootNode.hasError, false);
});
