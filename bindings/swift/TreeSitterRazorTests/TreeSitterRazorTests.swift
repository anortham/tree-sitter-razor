import Foundation
import XCTest
import SwiftTreeSitter
import TreeSitterRazor

final class TreeSitterRazorTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_razor())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Razor grammar")
    }

    func testParsesMixedRazorDocument() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_razor())
        try parser.setLanguage(language)

        let source = """
        @page
        <EditForm Model="@model"><InputDate @bind-Value="model.Date" /></EditForm>
        """
        let tree = try XCTUnwrap(parser.parse(source))
        let root = try XCTUnwrap(tree.rootNode)

        XCTAssertEqual(root.nodeType, "compilation_unit")
        XCTAssertFalse(root.hasError)
    }

    func testLoadsPackagedHighlightQuery() throws {
        let language = Language(language: tree_sitter_razor())
        let testBundle = Bundle(for: Self.self)
        let resourceBundleURL = testBundle.bundleURL
            .deletingLastPathComponent()
            .appendingPathComponent("TreeSitterRazor_TreeSitterRazor.bundle")
        let resourceBundle = try XCTUnwrap(Bundle(url: resourceBundleURL))
        let queriesURL = try XCTUnwrap(
            resourceBundle.url(forResource: "queries", withExtension: nil)
        )
        let configuration = try LanguageConfiguration(
            language,
            name: "Razor",
            queriesURL: queriesURL
        )

        XCTAssertNotNil(configuration.queries[.highlights])
    }
}
