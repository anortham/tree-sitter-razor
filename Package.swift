// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "TreeSitterRazor",
    products: [
        .library(name: "TreeSitterRazor", targets: ["TreeSitterRazor"]),
    ],
    dependencies: [
        .package(url: "https://github.com/ChimeHQ/SwiftTreeSitter", from: "0.8.0"),
    ],
    targets: [
        .target(
            name: "TreeSitterRazor",
            dependencies: [],
            path: ".",
            sources: [
                "src/parser.c",
                "src/scanner.c",
            ],
            resources: [
                .copy("queries")
            ],
            publicHeadersPath: "bindings/swift",
            cSettings: [.headerSearchPath("src")]
        ),
        .testTarget(
            name: "TreeSitterRazorTests",
            dependencies: [
                "SwiftTreeSitter",
                "TreeSitterRazor",
            ],
            path: "bindings/swift/TreeSitterRazorTests"
        )
    ],
    cLanguageStandard: .c11
)
