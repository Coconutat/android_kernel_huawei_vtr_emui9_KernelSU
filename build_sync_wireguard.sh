#!/bin/bash

if [ -d "net/wireguard" ]; then
	rm -rf net/wireguard
fi
bash scripts/fetch-latest-wireguard.sh