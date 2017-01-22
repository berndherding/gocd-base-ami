#!/bin/bash

[ -z "$AMI_COMMONS_INCLUDED" ] && AMI_COMMONS_INCLUDED=1 || return



function getImageId() {
  if [ -n "$GO_PACKAGE_AMI_REPO_GO_AMI_LABEL" ] ; then
    echo "$GO_PACKAGE_AMI_REPO_GO_AMI_LABEL"
  else
    cat "$(dirname "${BASH_SOURCE[0]}")/../target/imageId"
  fi
}



function destroyStack() {
  local stackname=$1

  aws cloudformation delete-stack --stack-name "$stackname" || return 1

  echo "*** please wait for delete-stack to complete. this may take a few minutes."
  aws cloudformation wait stack-delete-complete --stack-name "$stackname" --output text || return 2
}



function getDefaultVpcId() {
  aws ec2 describe-vpcs --filters Name=is-default,Values=true --query 'Vpcs[0].VpcId' --output text
}



function getStackname() {
  local stack=$1
  local label=$2

  counter="${GO_PIPELINE_COUNTER:-0}"

  stackname="gocd-$label-$stack-$counter"

  echo "$stackname"
}
