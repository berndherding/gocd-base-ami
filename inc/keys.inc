#!/bin/bash

[ -z "$GOCD_KEYS_INCLUDED" ] && GOCD_KEYS_INCLUDED=1 || return



function checkConfigFile() {
  if [ -f "$TARGET/gocd.config" ] ; then
    # shellcheck source=../target/gocd.config disable=SC1091
    . "$TARGET/gocd.config"
  fi
}



function getArmoredMultiLine() {

  echo "$1" | awk '
    {
      gsub(" ","\n")
      print
    }
  ' | awk '
    /^-----/ {
      p=1
    }
    p==1 {
      printf "%s ", $0
    }
    p==0 {
      print
    }
    /-----$/ {
      printf "\n"
      p=0
    }
  '
}



function init() {

  TARGET="$(dirname "${BASH_SOURCE[0]}")/../target"
  [ ! -d "$TARGET" ] && mkdir -p "$TARGET"
  export TARGET

  GOCD_PRIVATE_KEY_FILE=$TARGET/gocd.pem

  OPENSSL="$(which openssl)"

  checkConfigFile  
 
  SSH="ssh -i $(getGoCDPrivateKeyFile) -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"; export SSH
  SCP="scp -i $(getGoCDPrivateKeyFile) -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"; export SCP
}



function getDockerHubUsername() {
  [ -z "$DOCKER_HUB_USERNAME" ] && setDockerHubUsername
  echo "$DOCKER_HUB_USERNAME"
}



function setDockerHubUsername() {
  [ -n "$DOCKER_HUB_USERNAME" ] && return
  read -r -p "DOCKER_HUB_USERNAME: " DOCKER_HUB_USERNAME
  export DOCKER_HUB_USERNAME
}



function getDockerHubPassword() {
  [ -z "$DOCKER_HUB_PASSWORD" ] && setDockerHubPassword
  echo "$DOCKER_HUB_PASSWORD"
}



function setDockerHubPassword() {
  [ -n "$DOCKER_HUB_PASSWORD" ] && return 
  read -r -p "DOCKER_HUB_PASSWORD: " DOCKER_HUB_PASSWORD
  export DOCKER_HUB_PASSWORD
}



function getDockerHubEmail() {
  [ -z "$DOCKER_HUB_EMAIL" ] && setDockerHubEmail
  echo "$DOCKER_HUB_EMAIL"
}



function setDockerHubEmail() {
  [ -n "$DOCKER_HUB_EMAIL" ] && return
  read -r -p "DOCKER_HUB_EMAIL: " DOCKER_HUB_EMAIL
  export DOCKER_HUB_EMAIL
}



function getGoCDPrivateKeyFile() {
  [ ! -f "$GOCD_PRIVATE_KEY_FILE" ] && provideGoCDPrivateKeyFile
  echo "$GOCD_PRIVATE_KEY_FILE"
  # TODO: is there a pub key uploaded to AWS for this priv key? if not, generate one
  # with pipeline .no and date and change GOCD_PRIVATE_KEY_FILE name accordingly
}



function provideGoCDPrivateKeyFile() {
  [ -f "$GOCD_PRIVATE_KEY_FILE" ] && return 
  getGoCDPrivateKey > "$GOCD_PRIVATE_KEY_FILE"
  chmod 0400 "$GOCD_PRIVATE_KEY_FILE"
  export GOCD_PRIVATE_KEY_FILE
}



function getGoCDPrivateKey() {
  [ -z "$GOCD_PRIVATE_KEY" ] && setGoCDPrivateKey
  getArmoredMultiLine "$GOCD_PRIVATE_KEY" || return 1
}



function setGoCDPrivateKey() {
  [ -n "$GOCD_PRIVATE_KEY" ] & return
  GOCD_PRIVATE_KEY="$($OPENSSL genrsa 2048)" || return 2
  export GOCD_PRIVATE_KEY
}



function getGithubPrivateKey() {
  [ -z "$GITHUB_PRIVATE_KEY" ] && return 3
  getArmoredMultiLine "$GITHUB_PRIVATE_KEY" || return 4
}



init

#####

function provideSshPubKeyFile() {

  if [ -z "$SSH_PUB_KEY" ] ; then

    "$OPENSSL" rsa -in "$SSH_PEM_KEY_FILE" -pubout > "$SSH_PUB_KEY_FILE" || return 5

    json=$(aws ec2 describe-key-pairs --key-name gocd 2> /dev/null)

    keyFingerprint=${json##*KeyFingerprint\": \"}
    keyFingerprint=${keyFingerprint%%\"*}

    # TODO: check here for identity if key already exists
    # get fingerprint: openssl pkey -in target/old/gocd.pem -pubout -outform DER | openssl md5 -c
    # NOTE: openssl must be >= 1.0.2
    # if exists and is different: maybe add new key with name gocd-<pipelinecount>

    if [ -z "$keyFingerprint" ] ; then 
      aws ec2 import-key-pair --key-name gocd --public-key-material "$(awk '! /PUBLIC KEY/ {printf $0}' < "$SSH_PUB_KEY_FILE")"
      [ $? -ne 0 ] && return 7
    fi

    SSH_PUB_KEY="$(cat "$SSH_PUB_KEY_FILE")"

    export SSH_PUB_KEY
  fi

  if [ ! -f "$SSH_PUB_KEY_FILE" ] ; then
    echo "$SSH_PUB_KEY" >> "$SSH_PUB_KEY_FILE"
  fi
}
