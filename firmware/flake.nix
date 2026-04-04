{
  description = "CyberRC firmware development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          protobuf
          grpcio-tools
        ]);
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            platformio-core
            pythonEnv
          ];

          shellHook = ''
            echo "CyberRC Firmware Development Environment"
            echo ""
            echo "Commands:"
            echo "  pio run              - Build firmware"
            echo "  pio run -t upload    - Build and upload"
            echo "  pio run -t clean     - Clean build"
            echo "  pio device list      - List connected devices"
            echo ""
          '';
        };
      }
    );
}
