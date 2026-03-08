#!/usr/bin/env bash
set -e

if [ $# -lt 1 ]; then
  echo "usage: $0 <example.cr>"
  exit 1
fi

./build/caracal "$1"