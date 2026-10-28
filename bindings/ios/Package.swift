// swift-tools-version:5.7
import PackageDescription

let package = Package(
    name: "OlineriSDK",
    platforms: [
        .iOS(.v13),
        .macOS(.v11)
    ],
    products: [
        .library(
            name: "OlineriSDK",
            targets: ["OlineriSDK"]
        ),
    ],
    targets: [
        .target(
            name: "COlineri",
            path: "Sources/COlineri",
            sources: [
                "../../../../core/src/crypto.cpp",
                "../../../../core/src/manifest.cpp",
                "../../../../core/src/package_manager.cpp",
                "../../../../core/src/scoring_engine.cpp",
                "../../../../core/src/g2p.cpp",
                "../../../../core/src/runtime.cpp",
                "../../../../backends/mock/mock_engine.cpp"
            ],
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("../../../../core/include"),
                .headerSearchPath("../../../..")
            ]
        ),
        .target(
            name: "OlineriSDK",
            dependencies: ["COlineri"],
            path: "Sources/OlineriSDK"
        ),
        .testTarget(
            name: "OlineriSDKTests",
            dependencies: ["OlineriSDK"],
            path: "Tests/OlineriSDKTests"
        )
    ],
    cxxLanguageStandard: .cxx17
)
