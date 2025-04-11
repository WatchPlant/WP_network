#!/bin/bash


IMAGE_NAME=mqtt_web_app
IMAGE_TAG=latest

export DOCKER_BUILDKIT=1
docker build \
  --ssh default \
  -t $IMAGE_NAME:$IMAGE_TAG \
  -f Dockerfile .

