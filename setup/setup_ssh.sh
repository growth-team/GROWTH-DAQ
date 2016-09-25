#!/usr/bin/env bash

pushd $HOME
mkdir .ssh &> /dev/null
ssh-keygen << EOF







EOF

if [ ! -f authorized_keys ]; then
touch authorized_keys
fi

popd
