// swift-tools-version:5.7
import PackageDescription

let package = Package(
    name: "VaaniSDK",
    platforms: [
        .iOS(.v13),
        .macOS(.v11)
    ],
    products: [
        .library(
            name: "VaaniSDK",
            targets: ["VaaniSDK"]
        ),
    ],
    targets: [
        .target(
            name: "CVaani",
            path: "Sources/CVaani",
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
            name: "VaaniSDK",
            dependencies: ["CVaani"],
            path: "Sources/VaaniSDK"
        ),
        .testTarget(
            name: "VaaniSDKTests",
            dependencies: ["VaaniSDK"],
            path: "Tests/VaaniSDKTests"
        )
    ],
    cxxLanguageStandard: .cxx17
)
