#!/usr/bin/env bash

# shellcheck source=../ami.inc
. "$(dirname "${BASH_SOURCE[0]}")/../ami.inc"

STACKNAME=test-ami

_GO_PIPELINE_COUNTER=-${GO_PIPELINE_COUNTER:-0}

STACKNAME=$STACKNAME$_GO_PIPELINE_COUNTER

SHUNIT=$(which shunit)



function testCreateAmi() {
  createAmi "$STACKNAME"
  assertEquals "createAmi failed" 0 $?
}



function testInstanceIsInRunningState() {
  instanceId=$(aws ec2 describe-instances \
    --filters \
      Name=tag:Name,Values="$STACKNAME" \
      Name=instance-state-name,Values=running \
    --query "Reservations[*].Instances[*].InstanceId" \
    --output text
  )
  assertEquals "instance not in running state" 0 $?
  assertNotNull "running instance not found" "$instanceId"
}



#function testImageWasCreated() {}
#function testSnapshotWasCreated() {}
#function testEcsConfigIsThere() {}



function testDestroyAmi() {
  destroyAmi "$STACKNAME"
  assertEquals "destroyAmi failed" 0 $?
  # check if snapshot destroyed
}



function testInstanceIsInTerminatedState() {
  instanceId=$(aws ec2 describe-instances \
    --filters \
      Name=tag:Name,Values="$STACKNAME" \
      Name=instance-state-name,Values=terminated \
    --query "Reservations[*].Instances[*].InstanceId" \
    --output text
  )
  assertEquals "instance not in terminated state" 0 $?
  assertNotNull "terminated instance not found" "$instanceId"
}



# function testImageWasDestroyed() {}
# function testSnapshotWasDestroyed() {}



# function oneTimeSetUp() {
# function oneTimeTearDown()
# function setup()
# function teardown()



# shellcheck source=/usr/local/bin/shunit
. "$SHUNIT"
