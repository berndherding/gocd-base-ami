#!/usr/bin/env bash

# shellcheck source=ami.inc
. "$(dirname "${BASH_SOURCE[0]}")/ami.inc"

STACKNAME=gocd-ami

_GO_PIPELINE_COUNTER=-${GO_PIPELINE_COUNTER:-0}

STACKNAME=$STACKNAME$_GO_PIPELINE_COUNTER

destroyAmi "${1:-$STACKNAME}"
