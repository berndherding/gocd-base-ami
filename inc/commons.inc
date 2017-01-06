#!/bin/bash

[ -z "$AMI_COMMONS_INCLUDED" ] && AMI_COMMONS_INCLUDED=1 || return



function awsWaitNotEquals() {
  local value=$1
  local what=$2

  seconds=0
  # shellcheck disable=SC2086
  x=$(aws $what) || return 3

  while [[ "$x" -ne "$value" ]] && [[ $seconds -lt 600 ]] ; do
    sleep 15
    seconds=$((seconds+ 15))
    # shellcheck disable=SC2086
    x=$(aws $what) || return 4
  done

  [ "$x" -eq "$value" ] && return 0

  return 5
}



function awsWaitEquals() {
  local value=$1
  local what=$2

  seconds=0
  # shellcheck disable=SC2086
  x=$(aws $what) || return 6

  while [[ "$x" -eq "$value" ]] && [[ $seconds -lt 600 ]] ; do
    sleep 15
    seconds=$((seconds+ 15))
    # shellcheck disable=SC2086
    x=$(aws $what) || return 7
  done

  [ "$x" -eq "$value" ] && return 8

  return 0
}



function destroyStack() {
  local stackname=${1:-$DEFAULT_STACKNAME}

  aws cloudformation delete-stack --stack-name "$stackname" || return 1

  echo "*** please wait for delete-stack to complete. this may take a few minutes."
  aws cloudformation wait stack-delete-complete --stack-name "$stackname" --output text || return 2
}



function getBaseName() {
  local name=$1
  echo "${name%-+([0-9])}"
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



function getImageId() {
  if [ -n "$GO_PACKAGE_AMI_REPO_GO_AMI_LABEL" ] ; then
    echo "$GO_PACKAGE_AMI_REPO_GO_AMI_LABEL"
  else
    cat "$(dirname "$BASH_SOURCE")/target/imageId"
  fi
}



function getImageTag() {
  local imageName=$1

  imageTag=$imageName:${GO_PIPELINE_COUNTER:-0}

  if [ "$imageTag" != "$imageName:0" ] ; then

    gitHash=${GO_REVISION_FNS_GOCD_SERVER_DOCKER:0:7}
    gitHash=${gitHash:-$(git rev-parse --short HEAD)}

    imageTag="$imageTag-$gitHash"
  fi

  echo "$imageTag"
}



function getInstanceId() {
  local name=$1

  aws ec2 describe-instances \
  --filters "Name=tag:Name,Values=$name" "Name=instance-state-name,Values=running" \
  --query "Reservations[*].Instances[*].InstanceId" \
  --output text
}



function getPrivateIpAddress() {
  local instanceName=$1

  aws ec2 describe-instances \
  --filters \
    "Name=tag:Name,Values=$instanceName" \
    "Name=instance-state-name,Values=running" \
  --query "Reservations[*].Instances[*].PrivateIpAddress" \
  --output text
}



function getPublicDnsName() {
  local instanceName=$1

  aws ec2 describe-instances \
  --filters \
    "Name=tag:Name,Values=$instanceName" \
    "Name=instance-state-name,Values=running" \
  --query "Reservations[*].Instances[*].PublicDnsName" \
  --output text
}



function getPublicIpAddress() {
  local instanceName=$1

  aws ec2 describe-instances \
  --filters \
    "Name=tag:Name,Values=$instanceName" \
    "Name=instance-state-name,Values=running" \
  --query "Reservations[*].Instances[*].PublicIpAddress" \
  --output text
}



function getVersionNumber() {
  local name=$1
  echo "${name#$(getBaseName "$name")}"
}



function getVolumeId() {
  local goVolumeName=$1
  local status=${2:-available}

  aws ec2 describe-volumes \
  --filters Name=status,Values="$status" Name=tag:Name,Values="$goVolumeName" \
  --query "Volumes[*].VolumeId" \
  --output text
}
