#!/bin/bash

[ -z "$GOCD_KEYS_INCLUDED" ] && GOCD_KEYS_INCLUDED=1 || return



function initKeys() {

  initGoCD

  initFileNames

  getValueKeys    || return $?
  provideFileKeys || return $?
}



function initGoCD() {

  TARGET=/gocd
  GOCD=/gocd

  [ ! -d $TARGET ] && TARGET="$DIR/target"
  [ ! -d $TARGET ] && mkdir -p $TARGET

  export TARGET
  export GOCD
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



function getValueKeys() {
  setDockerHubEmail         || return $?
  setDockerHubUsername      || return $?
  setDockerHubPassword      || return $?
}



function setDockerHubEmail() {

  if [ -z "$DOCKER_HUB_EMAIL" ] ; then

    if [ ! -f "$DOCKER_HUB_EMAIL_FILE" ] ; then
      read -p "DOCKER_HUB_EMAIL: " DOCKER_HUB_EMAIL
      echo "$DOCKER_HUB_EMAIL" > "$DOCKER_HUB_EMAIL_FILE"
    fi

    DOCKER_HUB_EMAIL="$(cat "$DOCKER_HUB_EMAIL_FILE")"
    chmod 600 "$DOCKER_HUB_EMAIL_FILE"

    export DOCKER_HUB_EMAIL
  fi
}



function getDockerHubEmail() {
  echo "$DOCKER_HUB_EMAIL"
}



function setDockerHubUsername() {

  if [ -z "$DOCKER_HUB_USERNAME" ] ; then

    if [ ! -f "$DOCKER_HUB_USERNAME_FILE" ] ; then
      read -p "DOCKER_HUB_USERNAME: " DOCKER_HUB_USERNAME
      echo "$DOCKER_HUB_USERNAME" > "$DOCKER_HUB_USERNAME_FILE"
    fi

    DOCKER_HUB_USERNAME="$(cat "$DOCKER_HUB_USERNAME_FILE")"
    chmod 600 "$DOCKER_HUB_USERNAME_FILE"

    export DOCKER_HUB_USERNAME
  fi
}



function getDockerHubUsername() {
  echo "$DOCKER_HUB_USERNAME"
}



function setDockerHubPassword() {

  if [ -z "$DOCKER_HUB_PASSWORD" ] ; then

    if [ ! -f "$DOCKER_HUB_PASSWORD_FILE" ] ; then
      read -p "DOCKER_HUB_PASSWORD: " DOCKER_HUB_PASSWORD
      echo "$DOCKER_HUB_PASSWORD" > "$DOCKER_HUB_PASSWORD_FILE"
    fi

    DOCKER_HUB_PASSWORD="$(cat "$DOCKER_HUB_PASSWORD_FILE")"
    chmod 400 "$DOCKER_HUB_PASSWORD_FILE"

    export DOCKER_HUB_PASSWORD
  fi
}



function getDockerHubPassword() {
  echo "$DOCKER_HUB_PASSWORD"
}



function initFileNames() {

  SSH_PEM_KEY_FILE=$TARGET/gocd.pem
  SSH_PUB_KEY_FILE=$TARGET/gocd.pub

  export SSH="ssh -i $SSH_PEM_KEY_FILE -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"
  export SCP="scp -i $SSH_PEM_KEY_FILE -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"

  GITHUB_PEM_KEY_FILE=$TARGET/github.pem

  DOCKER_HUB_EMAIL_FILE=$TARGET/dockerHubEmail
  DOCKER_HUB_USERNAME_FILE=$TARGET/dockerHubUsername
  DOCKER_HUB_PASSWORD_FILE=$TARGET/dockerHubPassword
}



function provideFileKeys() {

  provideSshPemKeyFile    || return $?
  provideSshPubKeyFile    || return $?
  provideGithubPemKeyFile || return $?
}



function provideSshPemKeyFile() {

  if [ -z "$SSH_PEM_KEY" ] ; then

    if [ -n "$GOCD_SSH_PRIVATE_KEY" ] ; then
      getArmoredMultiLine "$GOCD_SSH_PRIVATE_KEY" > "$SSH_PEM_KEY_FILE"
    elif [ ! -f "$SSH_PEM_KEY_FILE" ] ; then
      openssl genrsa -out "$SSH_PEM_KEY_FILE" 2048 || return 4
    fi

    SSH_PEM_KEY="$(cat "$SSH_PEM_KEY_FILE")"

    export SSH_PEM_KEY
  fi

  if [ ! -f "$SSH_PEM_KEY_FILE" ] ; then
    echo "$SSH_PEM_KEY" >> "$SSH_PEM_KEY_FILE"
  fi

  # outside if stmt - GoCD disregards permission modes
  chmod 0400 "$SSH_PEM_KEY_FILE"
}



function provideSshPubKeyFile() {

  if [ -z "$SSH_PUB_KEY" ] ; then

    openssl rsa -in "$SSH_PEM_KEY_FILE" -pubout > "$SSH_PUB_KEY_FILE" || return 5

    json=$(aws ec2 describe-key-pairs --key-name gocd 2> /dev/null)

    keyFingerprint=${json##*KeyFingerprint\": \"}
    keyFingerprint=${keyFingerprint%%\"*}

    # TODO: check here for identity if key already exists
    # get fingerprint: openssl pkcs8 -in backup/gocd.pem -topk8 -nocrypt -outform DER | openssl sha1 -c
    # if exists and is different: maybe add new key with name gocd-<pipelinecount>

    if [ -z "$keyFingerprint" ] ; then 
      aws ec2 import-key-pair --key-name gocd --public-key-material "$(cat "$SSH_PUB_KEY_FILE" | awk '! /PUBLIC KEY/ {printf $0}')"
      [ $? -ne 0 ] && return 7
    fi

    SSH_PUB_KEY="$(cat "$SSH_PUB_KEY_FILE")"

    export SSH_PUB_KEY
  fi

  if [ ! -f "$SSH_PUB_KEY_FILE" ] ; then
    echo "$SSH_PUB_KEY" >> "$SSH_PUB_KEY_FILE"
  fi
}



function provideGithubPemKeyFile() {

  if [ -z "$GITHUB_PEM_KEY" ] ; then

    if [ -n "$GOCD_GITHUB_PRIVATE_KEY" ] ; then
      getArmoredMultiLine "$GOCD_GITHUB_PRIVATE_KEY" > "$GITHUB_PEM_KEY_FILE"
    elif [ ! -f "$GITHUB_PEM_KEY_FILE" ] ; then
      return 8
    fi

    GITHUB_PEM_KEY="$(cat "$GITHUB_PEM_KEY_FILE")"

    export GITHUB_PEM_KEY
  fi

  if [ ! -f "$GITHUB_PEM_KEY_FILE" ] ; then
    echo "$GITHUB_PEM_KEY" >> "$GITHUB_PEM_KEY_FILE"
  fi

  # outside if stmt - GoCD disregards permission modes
  chmod 0400 "$GITHUB_PEM_KEY_FILE"
}



initKeys || return $?
