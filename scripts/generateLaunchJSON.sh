#!/usr/bin/env bash
SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

cp "${SCRIPT_PATH}/launch.json" "${SCRIPT_PATH}/../.vscode/launch.json"