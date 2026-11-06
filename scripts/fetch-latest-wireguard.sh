#!/bin/bash
# 拉取 wireguard-linux-compat 最新源码到 net/wireguard。
#
# 与原版的区别（重要）：
#   原版先 `rm -rf net/wireguard` 再下载，网络不通时直接把源码删光，树就再也编不了。
#   这里改成"先下到临时目录，校验通过后再原子替换"，任何失败都保留现有源码。
#
# 仍然遵守原版的两个约定：
#   * flock .wireguard-fetch-lock —— 并发 make 只跑一个
#   * .check 时间戳 —— 24 小时内不重复联网
set -e

USER_AGENT="WireGuard-AndroidROMBuild/0.3 ($(uname -a))"

exec 9>.wireguard-fetch-lock
flock -n 9 || exit 0

[[ $(( $(date +%s) - $(stat -c %Y "net/wireguard/.check" 2>/dev/null || echo 0) )) -gt 86400 ]] || exit 0

DISTROS=$(curl -A "$USER_AGENT" -LSs --connect-timeout 10 https://build.wireguard.com/distros.txt || true)
if [[ -z "$DISTROS" ]]; then
	echo "wireguard: 无法获取 distros.txt（离线？），保留现有源码" >&2
	exit 0
fi

VERSION=
while read -r distro package version _; do
	if [[ $distro == upstream && $package == linuxcompat ]]; then
		VERSION="$version"
		break
	fi
done <<< "$DISTROS"

[[ -n $VERSION ]] || { echo "wireguard: distros.txt 里没有 linuxcompat 版本号，保留现有源码" >&2; exit 0; }

if [[ -f net/wireguard/version.h && $(< net/wireguard/version.h) == *$VERSION* ]]; then
	touch net/wireguard/.check
	exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if ! curl -A "$USER_AGENT" -LsS --connect-timeout 10 "https://git.zx2c4.com/wireguard-linux-compat/snapshot/wireguard-linux-compat-$VERSION.tar.xz" \
	| tar -C "$TMP" -xJf - --strip-components=2 "wireguard-linux-compat-$VERSION/src"; then
	echo "wireguard: 下载 $VERSION 失败，保留现有源码" >&2
	exit 0
fi

if [[ ! -f "$TMP/Kconfig" ]]; then
	echo "wireguard: 下载内容不完整（缺 Kconfig），保留现有源码" >&2
	exit 0
fi

sed -i 's/tristate/bool/;s/default m/default y/;' "$TMP/Kconfig"

rm -rf net/wireguard
mkdir -p net/wireguard
cp -a "$TMP/." net/wireguard/
touch net/wireguard/.check
