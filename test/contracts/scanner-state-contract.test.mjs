import assert from "node:assert/strict";
import { createRequire } from "node:module";
import test from "node:test";

const require = createRequire(import.meta.url);
const Parser = require("tree-sitter");
const Razor = require("../../bindings/node");

function createParser() {
  const parser = new Parser();
  parser.setLanguage(Razor);
  return parser;
}

function findNamedNode(node, type) {
  if (node.type === type) return node;
  for (const child of node.namedChildren) {
    const match = findNamedNode(child, type);
    if (match) return match;
  }
  return undefined;
}

function nestedMarkup(depth, text) {
  const names = Array.from({ length: depth }, (_, index) => `x-${index}`);
  return `${names.map((name) => `<${name}>`).join("")}${text}${names
    .toReversed()
    .map((name) => `</${name}>`)
    .join("")}`;
}

test("implicit expression ends before same-line text whitespace", () => {
  const source = "@person.Name is here today";
  const tree = createParser().parse(source);
  const expression = findNamedNode(tree.rootNode, "razor_implicit_expression");

  assert.equal(tree.rootNode.hasError, false);
  assert.equal(expression?.text, "@person.Name");
  assert.equal(expression?.endIndex, 12);
  assert.deepEqual(expression?.endPosition, { row: 0, column: 12 });
});

test("implicit expression ends before next-line text whitespace", () => {
  const source = "@person.Name\nmore text here";
  const tree = createParser().parse(source);
  const expression = findNamedNode(tree.rootNode, "razor_implicit_expression");

  assert.equal(tree.rootNode.hasError, false);
  assert.equal(expression?.text, "@person.Name");
  assert.equal(expression?.endIndex, 12);
  assert.deepEqual(expression?.endPosition, { row: 0, column: 12 });
});

test("implicit expression ends before a Razor block closing brace", () => {
  const source = '<div>@if (condition) { @Field("x", value) }</div>';
  const tree = createParser().parse(source);
  const conditional = findNamedNode(tree.rootNode, "razor_if");
  const expression = findNamedNode(tree.rootNode, "razor_implicit_expression");

  assert.equal(tree.rootNode.hasError, false);
  assert.equal(conditional?.text, '@if (condition) { @Field("x", value) }');
  assert.equal(expression?.text, '@Field("x", value)');
});

test("parses a 256-byte tag name", () => {
  const name = "x".repeat(256);
  const tree = createParser().parse(`<${name}>content</${name}>`);

  assert.equal(tree.rootNode.hasError, false);
});

test("parses and incrementally reparses 256 nested elements", () => {
  const parser = createParser();
  const source = nestedMarkup(256, "before");
  const tree = parser.parse(source);
  const startIndex = source.indexOf("before");
  const updated = `${source.slice(0, startIndex)}after!${source.slice(startIndex + 6)}`;

  assert.equal(tree.rootNode.hasError, false);

  tree.edit({
    startIndex,
    oldEndIndex: startIndex + 6,
    newEndIndex: startIndex + 6,
    startPosition: { row: 0, column: startIndex },
    oldEndPosition: { row: 0, column: startIndex + 6 },
    newEndPosition: { row: 0, column: startIndex + 6 },
  });

  const reparsed = parser.parse(updated, tree);

  assert.equal(reparsed.rootNode.hasError, false);
  assert.equal(reparsed.rootNode.text, updated);
});

test("keeps mismatched closing tags in error recovery", () => {
  const tree = createParser().parse("<outer><inner></outer></inner>");

  assert.equal(tree.rootNode.hasError, true);
});
