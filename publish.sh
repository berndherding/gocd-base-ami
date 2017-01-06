#!/usr/bin/env bash

# shellcheck source=ami.inc
. "$(dirname "$BASH_SOURCE")/ami.inc"

STACKNAME=gocd-ami

_GO_PIPELINE_COUNTER=-${GO_PIPELINE_COUNTER:-0}

STACKNAME=$STACKNAME$_GO_PIPELINE_COUNTER

publishAmi "${1:-$STACKNAME}"
