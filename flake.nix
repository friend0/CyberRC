# flake.nix
{
  description = "Development environment for PlatformIO, Arduino, and Teensy";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";  # You can also specify a channel here, like `nixpkgs/nixos-unstable`.

outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: let
    pkgs = import nixpkgs { inherit system; };
  in {
    # Define a devShell for development
    devShell = pkgs.mkShell {
      buildInputs = [
        pkgs.python3               # Python 3, required for PlatformIO
        pkgs.platformio            # PlatformIO CLI for embedded development
        pkgs.arduino-cli           # Arduino CLI for board and library management
        pkgs.teensy-loader-cli     # Teensy CLI loader for uploading to Teensy boards
        pkgs.gcc                   # Standard GCC toolchain
        pkgs.gcc-arm-embedded      # ARM GCC toolchain, necessary for ARM-based boards like Teensy
        pkgs.gnumake               # Make tool, sometimes required for build processes
        pkgs.openssl               # OpenSSL for SSL dependencies with PlatformIO
        pkgs.protobuf
        pkgs.cmake
        pkgs.rustup
        pkgs.zsh
      ];

      shellHook = ''
        # Start Zsh if not already the active shell
        if [ "$SHELL" != "$(command -v zsh)" ]; then
          export SHELL="$(command -v zsh)"
          exec zsh
        fi
        echo "Initializing PlatformIO, Arduino, and Teensy development environment..."
        platformio --version
        arduino-cli version
      '';
    };
  });
}
