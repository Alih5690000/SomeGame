em++.bat main.cpp -o main.html \
  -s USE_SDL=2 \
  -s USE_SDL_TTF=2 \
  -s USE_SDL_IMAGE=2 \
  -s SDL2_IMAGE_FORMATS='["png","jpg"]' \
  -s USE_SDL_MIXER=2 \
  -s SDL2_MIXER_FORMATS='["ogg","mp3","wav"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  --preload-file assets \
  -O2