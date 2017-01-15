#!/usr/bin/env bash

# shellcheck source=inc/commons.inc
. "$(dirname "${BASH_SOURCE[0]}")/inc/commons.inc"

[ ! -d "$(dirname "${BASH_SOURCE[0]}")/target" ] && mkdir "$(dirname "${BASH_SOURCE[0]}")/target"

ERROR_createAmiInstance_createStack=1
ERROR_createAmiInstance_waitStack=2
ERROR_createImage_createImage=3
ERROR_createImage_waitImage=4
ERROR_destroyImage_describeImages_imageId=5
ERROR_destroyImage_describeImages_snapshotId=6
ERROR_destroyImage_deregisterImage=7
ERROR_destroyImage_deleteSnapshot=8
ERROR_labelSnapshots_describeImages=9
ERROR_labelSnapshots_createTags=10



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



function getInstanceId() {
  local name=$1

  aws ec2 describe-instances \
  --filters "Name=tag:Name,Values=$name" "Name=instance-state-name,Values=running" \
  --query "Reservations[*].Instances[*].InstanceId" \
  --output text
}



function createAmiInstance() {
  local stackname=$1
  local dirname

  dirname="$(dirname "${BASH_SOURCE[0]}")"
  
  aws cloudformation create-stack \
  --stack-name "$stackname" \
  --template-body file://"$dirname/ami.cf" \
  --parameters ParameterKey=VpcId,ParameterValue="$(getDefaultVpcId)" \
  || return $ERROR_createAmiInstance_createStack

  echo "*** please wait for create-stack to complete. this may take a few minutes."
  aws cloudformation wait stack-create-complete --stack-name "$stackname" --output text \
  || return $ERROR_createAmiInstance_waitStack
}



function createImage() {
  local stackname=$1
  local instanceId

  # TODO: replace instanceId with stackname
  instanceId="$(getInstanceId "$stackname")"

  details=$(aws ec2 create-image --instance-id "$instanceId" --name "$stackname") \
  || return $ERROR_createImage_createImage

  imageId=$(getFromDetails "$details" ImageId)

  echo "*** please wait for create-image to complete. this may take a few minutes."
  aws ec2 wait image-available --image-ids "$imageId" || return $ERROR_createImage_waitImage

  echo "$imageId" > "$(dirname "${BASH_SOURCE[0]}")/target/imageId"
}



function waitImageDeregistered() {
  local imageId=$1

  awsWaitNotEquals 0 "ec2 describe-images --image-ids $imageId --query "'length(Images[*])'"" || return $?
}



function destroyImage() {
  local stackname=$1

  imageId=$(aws ec2 describe-images \
    --owners self \
    --filters Name=name,Values="$stackname" \
    --query 'Images[*].ImageId' \
    --output text
  ) || return $ERROR_destroyImage_describeImages_imageId

  snapshotIds=$(aws ec2 describe-images \
    --owners self \
    --filters Name=name,Values="$stackname" \
    --query 'Images[*].BlockDeviceMappings[*].Ebs.SnapshotId' \
    --output text
  ) || return $ERROR_destroyImage_describeImages_snapshotId

  aws ec2 deregister-image --image-id "$imageId"        || return $ERROR_destroyImage_deregisterImage
  waitImageDeregistered "$imageId"                      || return $?

  for snapshotId in $snapshotIds ; do
    aws ec2 delete-snapshot --snapshot-id "$snapshotId" || return $ERROR_destroyImage_deleteSnapshot
  done
}



function waitSnapshotsAvailable() {
  local stackname=$1
  
  awsWaitEquals 0 "\
    ec2 describe-images \
    --owner self \
    --filter Name=name,Values=$stackname \
    --query "'length(Images[].BlockDeviceMappings[*].Ebs.SnapshotId)'" \
  " || return $?
}



function labelSnapshots() {
  local stackname=$1

  details=$(aws ec2 describe-images \
    --owners self \
    --filters Name=name,Values="$stackname" \
    --query 'Images[*].BlockDeviceMappings[*].{device: DeviceName, snapshot: Ebs.SnapshotId}' \
    --output text
  ) || return $ERROR_labelSnapshots_describeImages

  IFS=$'\n'

  for tuple in $details ; do
    unset IFS 

    col=($tuple)
    device=${col[0]}
    device=${device//\/dev\/}
    snapshotId=${col[1]}
    name="$stackname-$device"

    aws ec2 create-tags \
    --resources "$snapshotId" \
    --tags Key=Name,Value="$name" \
    || return $ERROR_labelSnapshots_createTags

  done
}



function createAmi() {
  local stackname=$1
  
  createAmiInstance "$stackname"      || return $?
  createImage "$stackname"            || return $?
  waitSnapshotsAvailable "$stackname" || return $?
  labelSnapshots "$stackname"         || return $?
}



function publishAmi() {
  local stackname=$1

  createAmi    "$stackname"
  destroyStack "$stackname"
}



function destroyAmi() {
  local stackname=$1

  destroyStack "$stackname" #|| return $?
  destroyImage "$stackname" || return $?
}
