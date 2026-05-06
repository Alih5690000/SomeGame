#!/usr/bin/env bash

set -e

echo "Installing Emscripten into current directory..."

# 1. Клонируем emsdk в ./emsdk

git clone https://github.com/emscripten-core/emsdk.git ./emsdk

cd emsdk

# 2. Устанавливаем и активируем latest

./emsdk install latest
./emsdk activate latest

# 3. Загружаем env

source ./emsdk_env.sh

# 4. Проверка

emcc -v

echo "Done!"
echo "To use later run: source ./emsdk/emsdk_env.sh"
