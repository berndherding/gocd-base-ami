#!/bin/bash

# shellcheck source=inc/commons.inc
. "$(dirname "$BASH_SOURCE")/inc/commons.inc"

[ ! -d "$(dirname "$BASH_SOURCE")/target" ] && mkdir "$(dirname "$BASH_SOURCE")/target"



function createAmiInstance() {
  local stackname=$1
  local dirname

  dirname="$(dirname "$BASH_SOURCE")"
  
  aws cloudformation create-stack \
  --stack-name "$stackname" \
  --template-body file://"$dirname/ami.cf" \
  --parameters ParameterKey=VpcId,ParameterValue="$(getDefaultVpcId)" \
  || return 1

  echo "*** please wait for create-stack to complete. this may take a few minutes."
  aws cloudformation wait stack-create-complete --stack-name "$stackname" --output text || return 2

  getInstanceId       "$stackname" > "$dirname/target/instanceId"
  getPublicIpAddress  "$stackname" > "$dirname/target/publicIpAddress"
}



function createImage() {
  local instanceId=$1

  # TODO: replace instanceId with stackname

  details=$(aws ec2 create-image --instance-id "$instanceId" --name "$stackname") || return 4

  imageId=$(getFromDetails "$details" ImageId)

  echo "*** please wait for create-image to complete. this may take a few minutes."
  aws ec2 wait image-available --image-ids "$imageId" || return 5

  echo "$imageId" > "$(dirname "$BASH_SOURCE")/target/imageId"
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
  ) || return 6

  snapshotIds=$(aws ec2 describe-images \
    --owners self \
    --filters Name=name,Values="$stackname" \
    --query 'Images[*].BlockDeviceMappings[*].Ebs.SnapshotId' \
    --output text
  ) || return 7

  aws ec2 deregister-image --image-id "$imageId"        || return 8
  waitImageDeregistered "$imageId"                      || return $?

  for snapshotId in $snapshotIds ; do
    aws ec2 delete-snapshot --snapshot-id "$snapshotId" || return 9
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
    --filters Name=name,Values=gocd-ami-0 \
    --query 'Images[*].BlockDeviceMappings[*].{device: DeviceName, snapshot: Ebs.SnapshotId}' \
    --output text
  ) || return 10

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
    || return 11

  done
}



function createAmi() {
  local stackname=$1
  
  instanceId="$(cat "$(dirname "$BASH_SOURCE")/target/instanceId")"

  createAmiInstance "$stackname"      || return $?
  createImage "$instanceId"           || return $?
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
