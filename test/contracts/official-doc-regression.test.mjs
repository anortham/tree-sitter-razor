import assert from "node:assert/strict";
import { createRequire } from "node:module";
import test from "node:test";

const require = createRequire(import.meta.url);
const Parser = require("tree-sitter");
const Razor = require("../../bindings/node");

function parse(source) {
  const parser = new Parser();
  parser.setLanguage(Razor);
  return parser.parse(source);
}

function findNamedNode(node, type) {
  if (node.type === type) return node;
  for (const child of node.namedChildren) {
    const match = findNamedNode(child, type);
    if (match) return match;
  }
  return undefined;
}

test("component value mixes literal text and an explicit expression", () => {
  const tree = parse('<ParameterChild Title="Set by @(panelData.Title)" />');
  const value = findNamedNode(tree.rootNode, "component_attribute_value");
  const expression = findNamedNode(value, "razor_explicit_expression");

  assert.equal(tree.rootNode.hasError, false);
  assert.equal(expression?.text, "@(panelData.Title)");
});

test("arrow expression clause returns a Razor template", () => {
  const tree = parse(
    "@code { private RenderFragment Fragment() => @<text>Hello</text>; }",
  );
  const clause = findNamedNode(tree.rootNode, "arrow_expression_clause");
  const template = findNamedNode(clause, "razor_template");

  assert.equal(tree.rootNode.hasError, false);
  assert.equal(template?.text, "@<text>Hello</text>");
});
