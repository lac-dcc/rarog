{
  description = "C++, LLVM 22, and MLIR 22 Development Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux"; # Change to aarch64-linux if on ARM
      pkgs = nixpkgs.legacyPackages.${system};

      # Prevent shadowing confusion
      llvmPackages = pkgs.llvmPackages_22;
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        # Build utilities from main pkgs
        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
        ];

        buildInputs =
          with llvmPackages;
          with pkgs;
          [
            clang
            mlir
            llvm
            libffi
            libxml2
          ];

        # Environment variables mapping tools and CMake paths
        shellHook = ''
          echo "=== LLVM/MLIR 22 Environment Loaded ==="

          export CC="${llvmPackages.clang}/bin/clang"
          export CXX="${llvmPackages.clang}/bin/clang++"

          export MLIR_DIR="${llvmPackages.mlir}/lib/cmake/mlir"
          export LLVM_DIR="${llvmPackages.llvm}/lib/cmake/llvm"

          clang --version
          mlir-opt --version
        '';
      };
    };
}
