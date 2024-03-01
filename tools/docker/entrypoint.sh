#!/bin/bash

if [ ! -d "${HOME}/.metabasenet/" ]; then
    mkdir -p ${HOME}/.metabasenet/
fi

if [ ! -f "${HOME}/.metabasenet/metabasenet.conf" ]; then
    cp /metabasenet.conf ${HOME}/.metabasenet/metabasenet.conf
fi

exec "$@"
