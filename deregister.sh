#!/usr/bin/env bash

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

# shellcheck source=ami.inc
. "$DIR/ami.inc"

STACKNAME=gocd-ami

_GO_PIPELINE_COUNTER=-${GO_PIPELINE_COUNTER:-0}

STACKNAME=$STACKNAME$_GO_PIPELINE_COUNTER

destroyAmi "${1:-$STACKNAME}"
