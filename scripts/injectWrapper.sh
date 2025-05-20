#!/usr/bin/env bash
SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

if [[ ! -e "${SCRIPT_PATH}/../result/share/spotify/.spotify-wrapped" ]]; then 
    echo "Spotify not installed, please build spotify using the nix flake first";
    exit 1;
fi

cp "${SCRIPT_PATH}/launch.json" "${SCRIPT_PATH}/../.vscode/launch.json"
