#!/bin/bash

[ -z "$AMI_COMMONS_INCLUDED" ] && AMI_COMMONS_INCLUDED=1 || return



function destroyStack() {
  local stackname=${1:-$DEFAULT_STACKNAME}

  aws cloudformation delete-stack --stack-name "$stackname" || return 1

  echo "*** please wait for delete-stack to complete. this may take a few minutes."
  aws cloudformation wait stack-delete-complete --stack-name "$stackname" --output text || return 2
}



function getDefaultVpcId() {
  aws ec2 describe-vpcs --filters Name=is-default,Values=true --query 'Vpcs[0].VpcId' --output text
}



function getFromDetails() {
  local details=$1
  local key=$2
  local s
  s=${details#*$key\"*\"}
  s=${s%%\"*}
  echo "$s"
}



function getStackname() {
  local stackprefix=$1
  local stacksuffix=$2
  local env stackname

  if [ -n "${stacksuffix##+([0-9])-*}" ] ; then
    stacksuffix="${GO_PIPELINE_COUNTER:-0}-$stacksuffix"
  fi

  stackname=$stackprefix-$stacksuffix

  echo "$stackname"
}
