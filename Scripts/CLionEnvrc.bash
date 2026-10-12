#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]:-$0}")/.." || return 1
eval "$(direnv export bash)"