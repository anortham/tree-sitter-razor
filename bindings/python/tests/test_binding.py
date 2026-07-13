from importlib.metadata import requires
from unittest import TestCase

import tree_sitter, tree_sitter_razor


RAZOR_SOURCE = b'@page\n<EditForm Model="@model"><InputDate @bind-Value="model.Date" /></EditForm>'


class TestLanguage(TestCase):
    def setUp(self):
        self.language = tree_sitter.Language(tree_sitter_razor.language())

    def test_can_parse_mixed_razor_document(self):
        tree = tree_sitter.Parser(self.language).parse(RAZOR_SOURCE)

        self.assertEqual(tree.root_node.type, "compilation_unit")
        self.assertFalse(tree.root_node.has_error)

    def test_can_load_packaged_highlight_query(self):
        query_source = tree_sitter_razor.HIGHLIGHTS_QUERY

        self.assertTrue(query_source)
        tree_sitter.Query(self.language, query_source)

    def test_declares_supported_tree_sitter_versions(self):
        tree_sitter_requirement = next(
            requirement
            for requirement in requires("tree-sitter-razor")
            if requirement.startswith("tree-sitter")
        )

        self.assertIn(">=0.25", tree_sitter_requirement)
        self.assertIn("<0.27", tree_sitter_requirement)
