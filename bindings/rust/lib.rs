//! This crate provides Razor language support for the [tree-sitter][] parsing library.
//!
//! Typically, you will use the [LANGUAGE][] constant to add this language to a
//! tree-sitter [Parser][], and then use the parser to parse some code:
//!
//! ```
//! let code = r#"
//! @page "/"
//! <h1>Hello, @person</h1>
//!
//! @code{
//! var person = "world";
//! }
//! "#;
//! let mut parser = tree_sitter::Parser::new();
//! let language = tree_sitter_razor::LANGUAGE;
//! parser
//!     .set_language(&language.into())
//!     .expect("Error loading Razor parser");
//! let tree = parser.parse(code, None).unwrap();
//! assert!(!tree.root_node().has_error());
//! ```
//!
//! [Parser]: https://docs.rs/tree-sitter/*/tree_sitter/struct.Parser.html
//! [tree-sitter]: https://tree-sitter.github.io/

use tree_sitter_language::LanguageFn;

extern "C" {
    fn tree_sitter_razor() -> *const ();
}

/// The tree-sitter [`LanguageFn`][LanguageFn] for this grammar.
///
/// [LanguageFn]: https://docs.rs/tree-sitter-language/*/tree_sitter_language/struct.LanguageFn.html
pub const LANGUAGE: LanguageFn = unsafe { LanguageFn::from_raw(tree_sitter_razor) };

/// The content of the [`node-types.json`][] file for this grammar.
///
/// [`node-types.json`]: https://tree-sitter.github.io/tree-sitter/using-parsers#static-node-types
pub const NODE_TYPES: &str = include_str!("../../src/node-types.json");

// NOTE: uncomment these to include any queries that this grammar contains:

pub const HIGHLIGHTS_QUERY: &str = include_str!("../../queries/highlights.scm");
pub const INJECTIONS_QUERY: &str = include_str!("../../queries/injections.scm");
// pub const LOCALS_QUERY: &str = include_str!("../../queries/locals.scm");
// pub const TAGS_QUERY: &str = include_str!("../../queries/tags.scm");

#[cfg(test)]
mod tests {
    #[test]
    fn test_can_load_grammar() {
        let mut parser = tree_sitter::Parser::new();
        parser
            .set_language(&super::LANGUAGE.into())
            .expect("Error loading Razor parser");
    }

    #[test]
    fn test_parses_mixed_razor_document() {
        let mut parser = tree_sitter::Parser::new();
        parser
            .set_language(&super::LANGUAGE.into())
            .expect("Error loading Razor parser");

        let source = r#"@page
<EditForm Model="@model"><InputDate @bind-Value="model.Date" /></EditForm>"#;
        let tree = parser.parse(source, None).expect("Razor parse failed");
        let root = tree.root_node();

        assert_eq!(root.kind(), "compilation_unit");
        assert!(!root.has_error(), "unexpected parse errors: {root}");
    }

    #[test]
    fn test_loads_packaged_resources() {
        assert!(!super::NODE_TYPES.trim().is_empty());
        tree_sitter::Query::new(&super::LANGUAGE.into(), super::HIGHLIGHTS_QUERY)
            .expect("Error loading Razor highlights query");
    }
}
