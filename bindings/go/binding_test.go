package tree_sitter_razor_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_razor "github.com/tree-sitter/tree-sitter-razor/bindings/go"
)

func TestCanParseMixedRazor(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_razor.Language())
	parser := tree_sitter.NewParser()
	t.Cleanup(parser.Close)
	if err := parser.SetLanguage(language); err != nil {
		t.Fatalf("set Razor language: %v", err)
	}

	tree := parser.Parse([]byte("@page\n<EditForm Model=\"@model\"><InputDate @bind-Value=\"model.Date\" /></EditForm>"), nil)
	t.Cleanup(tree.Close)
	root := tree.RootNode()
	if root.Kind() != "compilation_unit" {
		t.Fatalf("root kind = %q, want %q", root.Kind(), "compilation_unit")
	}
	if root.HasError() {
		t.Fatalf("parse contains errors: %s", root.ToSexp())
	}
}
